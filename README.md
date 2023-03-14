# Multithreaded HTTP Server with Atomic Requests and Coherent Logs

`httpserver` is a multithreaded http server that runs on localhost and serves atomic file requests from multiple clients. The server distributes work amongst threads by using a work queue and it coherently logs the order of requests. It implements two standard http methods (`PUT` and `GET`) and one non-standard http method (`APPEND`).

## Design

### 1. HTTP Methods
1. `GET`
   * `httpserver` recieves an object name and sends back the contents of that object to the client
2. `PUT`
   * `httpserver` recieves an object name and data content. It writes the data into the specified object if it exists, otherwise it creates it 
3. `APPEND`
   * `httpserver` recieves an object name and data content. It appends the data into the specified object if it exists

### 2. Client Response Codes
* `200 OK`
  * this response is sent to clients over the connection when a `GET`, `PUT`, or `APPEND` request is completed successfully
* `201 CREATED`
  * this response is sent to clients over the connection when a `PUT` request is successful and it created the requested file
* `400 BAD REQUEST`
  * this response is sent to clients over the connection when their request is ill formatted or missing necessary header fields
* `403 FORBIDDEN`
  * this response is sent to clients over the connection when the file they requested is not accessible (access permissions)
* `404 FILE NOT FOUND`
  * this response is sent to clients over the connection when the file they requested on a `GET` or `APPEND` does not exist
* `500 INTERNAL SERVER ERROR`
  * this response is sent to clients over the connection when an error is encountered within the functionality of the server
* `501 NOT IMPLEMENTED`
  * this response is sent to clients over the connection when the method they requested is not implemented or does not exist

### 3. Data Structures
`uint8_t buffer[2048/4096]` ->
* 2KB/4KB arrays of bytes are used to buffer file contents, as a medium for us to work faster on the contents of a file and write it out afterwards. They are also used to process the http request and run regular expression matchers on it

`struct request_t` ->
* the `request_t` struct contains a series of other structs and types that track meta data about the current request being serviced. Some of this meta data includes the request line, the content length, the request id, the status of the request, the current progress state, etc.

`struct header_reg` ->
* the `header_reg` struct contains the three regex objects that are used to match the http requests. There is one regular expression for the request line another for the header field lines, and another to find the end of the request.

`struct connection_t` ->
* the `connection_t` struct containts a `request_t` struct and the `connfd` of the current connection. I decided to make a distinction between connection and request. I felt that any client could connect to the server and initiate a connection, but only some clients will connect and give me a valid request. Therefore, what is passed around the workers and the queue is a connection, which contains a request and the socket fd of that connection.

`struct queue_t` ->
* a worker queue implemented as an underlying linked list structure (`linkedlist_t`). This queue is a generic `void *` queue, but is used to hold `connection_t` structures.

`struct connpoll_t` ->
* a connection poller structure, this structure uses `epoll` and it's underlying system calls to monitor a set of file descriptors for incoming events, specifically `EPOLLIN` (data coming in from the client). This is especially useful for slow connections that would block. A thread can push the slow connection onto the `connection poller` and `map_t` and let the kernel manage the `epoll` instance and let the dispatcher manage the connection `map_t`.

`struct threadpool_t` ->
* a thread pool struct that holds a pool of threads, the worker queue, a pointer to the connection `map_t` and `connection poller`, and some other meta data that the pool needs to function as its own module.

`struct map_t` -> 
* this is an ordered map implemented as an underlying red black tree (`redblack_t`). The map is used to map connection fds to `connection_t` structs, which track the status of a connection's request.

`struct linkedlist_t` ->
* a generic (void *) linked list with both insert front/back and pop front/back capabilities. This is the underlying data structure for the `queue_t`

`struct redblack_t` ->
* a redblack tree that maps connfds to `connection_t` structs. this is the underlying data structure for `map_t` (a typedef of `redblack_t`)

### 4. Locks and Condition Variables

`wqlock`, `wqnotify` ->
* `wqlock` is a mutex lock that guards the worker queue from race conditions and undefined behavior from multiple threads accessesing it at the same time
* `wqnotify` is a condition variable that notifies worker threads when there is work to be dequeued from the worker queue

`maplock` ->
* `maplock` is a mutex lock that guards operations on the connection map since both worker threads and the dispatcher thread add and delete entries

`filelock` ->
* `filelock` is a mutex lock that guards operations such as GET opening a file, PUT unlinking the old target file and renaming the temporary file to the unlinked target file, and APPEND appending the file contents from a temporary file to the target file. The idea was to make critical sections as small as I could to only share one lock. The rest of the request operations could be done outside of the critical region

### 5. Non-blocking IO/ event-driven IO
This httpserver makes use of non-blocking IO/ event-drive IO. Non-blocking means that if at any point the client socket would block on `recv` or `send`, then the worker thread will save the state of the current request and suspend it instead of waiting on the client. When it is suspended, it is sent to the main thread (dispatcher), where it is monitored for events that indicate the socket is ready for `receiving` or `sending`. This is the event-driven side of it.

### 6. Logging

the header field called `Request-Id` assigns an ID to each request that is sent. Upon completing a client's request, the request is logged in the `logfile` with the format:

* `METHOD,/OBJECT,CODE,REQ-ID`

This is supposed to happen atomically from the other logs, and coherently such that the order of the logged requests reflects the true order in which they were completed. This means that a GET request should recieve the last PUT contents of the file it is requesting, and not any other content.

### 7. Module Overview
Modules in this project:
01. `httpserver`
    * handles connections and sends the request to one of three handler functions: `handle_get`, `handle_put`, or `handle_append`
    * direct connections: `request`, `status`, `ioutil`, `util`, `connection`, `queue`, `redblack`, `linkedlist`, `connpoll`, `threadpool`
02. `request`
    * in charge of initializing `header_t` structs, parsing http requests, and validating http requests
    * direct connections: `re`, `status`, `util`, `connection`
03. `re`
    * initializes `header_reg` structs, compiles regular expressions, and deallocates regular expressions
    * direct connections: `request`
04. `status`
    * stores enumerations for status codes, macros for status messages, and a function that resolves which message to send
    * direct connections: `httpserver`, `request`, `ioutil`, `connection`
05. `ioutil`
    * contains functions for reading/writing to files, opening files, checking the size of a file, and checking if it's a directory
    * direct connections: `httpserver`, `util`
06. `util`
    * contains small utilities, like converting strings to uint16 and int64 (only positive), and converting strings to lowercase
    * direct connections: `httpserver`, `request`, `ioutil`
07. `connection`
    * a file that implements the `connection_t` structure for passing around connection information between threads
    * direct connections: `redblack`, `threadpool`, `connpoll`, `httpserver`, `request`
08. `linkedlist`
    * a generic `void *` linkedlist. A very versatile data structure for stacks, queues, and hashmap collision resolution
    * direct connections: `queue`
09.  `queue`
     * an unbounded queue with an underlying linkedlist structure. This queue servers as the dispatcher-worker queue for the threadpool
     * direct connections: `linkedlist`, `threadpool`, `httpserver`
10. `redblack`
    * a red-black tree structure used to map (`map_t`) suspended connection fds to their `connection_t` structure
    * direct connections: `connection`, `threadpool`, `httpserver`
11. `connpoll`
    * a wrapper around `epoll` that can add/delete connections to/from a set of monitored connections and yield any ready connections
    * direct connections: `connection`, `threadpool`, `httpserver`
12. `threadpool`
    * a threadpool which manages worker threads and the dispatcher-worker queue (adding work and thread work function). It also takes care of cleaning up threads on shutdown
    * direct connections: `connection`, `redblack`, `connpoll`, `httpserver`

### 8. High Level Overview
1. `httpserver` starts up on the command line: `./httpserver <portnumber> [-t threads] [-l logfile]`. It takes a portnumber for binding, a number of threads, and a logfile to log to.
2. a client like `curl(1)`, `netcat(1)`, or `olivertwist` is used to send sequential or concurrent requests to the `httpserver` over the binded port
3. `httpserver`'s main thread `poll`s the listening socket and recieves requests, which get enqueued to the `threadpool`s work queue. 
4. from there, threads working in the function `free_young_thug` either pick up a connection or wait on a condition variable if there is no work
5. after picking up a connection, the thread recieves the header, parses it, validates it, and if it is valid, it will service the request
6. if at any point in these stages the client socket would block, or the client is slow, the worker thread suspends the connection in the `map` and sends it to the main thread's `poll` to be monitored for events that indicate it is ready to proceed. The thread can then go on to service other requests
7. if a slow connection is ready, the main thread will extract the request from the `map` and push it back onto the worker queue to get serviced again from where it left off.
8. within `handle_get()`, `handle_put()`, or `handle_append()`, the requests are serviced, and any `400`, `403`, `404`, and `500` codes are handled for file errors or internal errors respectively
9.  upon success, the request is logged, and a `200` code is sent to the client for `GET` and `APPEND` and either a `200` is sent for `PUT`, or a `201` if the file was created
10. the client connection is then closed by the worker thread, and the worker thread continues to wait on the condition variable if there is no work or it goes on to service more requests. The main thread continues to `poll` the listening socket for more client connections and the incomplete requests for events on the socket
11. A SIGTERM signal can be invoked to shutdown the `httpserver` process, at which point the main thread will head over to the `sigterm_handler`, join all the threads in the `threadpool`, and free up all memory occupied by all the data structures

### 9. Limitations
1. `httpserver` does not work across different networks
2. `httpserver` ignores most header fields (ex: hostname)
3. `httpserver` only supports 2 standard http methods (`PUT` and `GET`) and 1 non-standard http method (`APPEND`)
4. `httpserver` is not completely "coherent" or "atomic" in logging

## Building

    $ make
    $ make all

## Running

    $ ./httpserver <portnumber> -t <threads> -l <logfile>
        * portnumber: binding port for httpserver to listen for requests and serve them
        * -t <threads>: number of threads running in the httpserver
        * -l <logfile>: specifies a logfile for output

## Formatting

    $ make format

## Tidy

    $ make tidy

## Cleaning

    $ make clean
