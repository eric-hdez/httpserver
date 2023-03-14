#include "redblack.h"
#include <stdlib.h>

// red black tree algorithms inspired by: Introduction to Algorithms, 3rd ed. by Cormen, Leiserson, Rivest, Stein

typedef enum { RED, BLACK } color_t;

struct rbnode_t {
    rbnode_t *parent, *left, *right;
    int key;
    connection_t *val;
    color_t color;
};

struct redblack_t {
    rbnode_t *root, *nil;
    size_t size;
};

rbnode_t *rbnode_create(int key, connection_t *val) {
    rbnode_t *n = malloc(sizeof(rbnode_t));
    if (n == NULL) {
        return n;
    }

    n->left = n->right = n->parent = NULL;
    n->key = key, n->val = val;
    n->color = RED;
    return n;
}

void rbnode_destroy(rbnode_t **n, void (*del_func)(void *)) {
    if (n && *n) {
        if (del_func != NULL) {
            if ((*n)->val != NULL) {
                del_func((void *) (*n)->val);
            }
        }

        free(*n);
        *n = NULL;
    }
}

rbnode_t *rb_tree_min(redblack_t *rbt, rbnode_t *x) {
    if (rbt->root == rbt->nil) {
        return rbt->nil;
    }

    rbnode_t *min = x;

    while (min->left != rbt->nil) {
        min = min->left;
    }

    return min;
}

rbnode_t *rb_tree_search(redblack_t *rbt, rbnode_t *n, int k) {
    if (n == rbt->nil || k == n->key) {
        return n;
    } else if (k < n->key) {
        return rb_tree_search(rbt, n->left, k);
    } else {
        return rb_tree_search(rbt, n->right, k);
    }
}

void rb_left_rotate(redblack_t *rbt, rbnode_t *x) {
    rbnode_t *y = x->right;

    x->right = y->left;
    if (y->left != rbt->nil) {
        y->left->parent = x;
    }

    y->parent = x->parent;
    if (x->parent == rbt->nil) {
        rbt->root = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }

    y->left = x;
    x->parent = y;
}

void rb_right_rotate(redblack_t *rbt, rbnode_t *x) {
    rbnode_t *y = x->left;

    x->left = y->right;
    if (y->right != rbt->nil) {
        y->right->parent = x;
    }

    y->parent = x->parent;
    if (x->parent == rbt->nil) {
        rbt->root = y;
    } else if (x == x->parent->right) {
        x->parent->right = y;
    } else {
        x->parent->left = y;
    }

    y->right = x;
    x->parent = y;
}

void rb_insert_fixup(redblack_t *rbt, rbnode_t *z) {
    while (z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
            rbnode_t *y = z->parent->parent->right;

            if (y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    rb_left_rotate(rbt, z);
                }

                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rb_right_rotate(rbt, z->parent->parent);
            }
        } else {
            rbnode_t *y = z->parent->parent->left;

            if (y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rb_right_rotate(rbt, z);
                }

                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rb_left_rotate(rbt, z->parent->parent);
            }
        }
    }

    rbt->root->color = BLACK;
}

void rb_insert(redblack_t *rbt, int key, connection_t *conn) {
    rbnode_t *y = rbt->nil;
    rbnode_t *x = rbt->root;

    while (x != rbt->nil) {
        y = x;

        if (key < x->key) {
            x = x->left;
        } else if (key > x->key) {
            x = x->right;
        } else {
            x->val = conn;
            return;
        }
    }

    rbnode_t *n = rbnode_create(key, conn);
    n->parent = y;

    if (y == rbt->nil) {
        rbt->root = n;
    } else if (n->key < y->key) {
        y->left = n;
    } else {
        y->right = n;
    }

    n->left = n->right = rbt->nil;
    n->color = RED;

    rb_insert_fixup(rbt, n);
    rbt->size += 1;
}

void rb_transplant(redblack_t *rbt, rbnode_t *u, rbnode_t *v) {
    if (u->parent == rbt->nil) {
        rbt->root = v;
    } else if (u == u->parent->left) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }

    v->parent = u->parent;
}

void rb_delete_fixup(redblack_t *rbt, rbnode_t *x) {
    while (x != rbt->root && x->color == BLACK) {
        if (x == x->parent->left) {
            rbnode_t *w = x->parent->right;

            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                rb_left_rotate(rbt, x->parent);
                w = x->parent->right;
            }

            if (w->left->color == BLACK && w->right->color == BLACK) {
                w->color = RED;
                x = x->parent;
            } else {
                if (w->right->color == BLACK) {
                    w->left->color = BLACK;
                    w->color = RED;
                    rb_right_rotate(rbt, w);
                    w = x->parent->right;
                }

                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->right->color = BLACK;
                rb_left_rotate(rbt, x->parent);
                x = rbt->root;
            }
        } else {
            rbnode_t *w = x->parent->left;

            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                rb_right_rotate(rbt, x->parent);
                w = x->parent->left;
            }

            if (w->right->color == BLACK && w->left->color == BLACK) {
                w->color = RED;
                x = x->parent;
            } else {
                if (w->left->color == BLACK) {
                    w->right->color = BLACK;
                    w->color = RED;
                    rb_left_rotate(rbt, w);
                    w = x->parent->left;
                }

                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->left->color = BLACK;
                rb_right_rotate(rbt, x->parent);
                x = rbt->root;
            }
        }
    }

    x->color = BLACK;
}

void rb_delete(redblack_t *rbt, rbnode_t *z, void (*del_func)(void *)) {
    rbnode_t *y = z;
    color_t y_original_color = y->color;
    rbnode_t *x;

    if (z->left == rbt->nil) {
        x = z->right;
        rb_transplant(rbt, z, z->right);
    } else if (z->right == rbt->nil) {
        x = z->left;
        rb_transplant(rbt, z, z->left);
    } else {
        rbnode_t *y = rb_tree_min(rbt, z->right);
        y_original_color = y->color;
        x = y->right;

        if (y->parent == z) {
            x->parent = y;
        } else {
            rb_transplant(rbt, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }

        rb_transplant(rbt, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }

    if (y_original_color == BLACK) {
        rb_delete_fixup(rbt, x);
    }

    rbnode_destroy(&z, del_func);
}

void rb_tree_destroy(redblack_t *rbt, rbnode_t **root) {
    if (root != NULL && *root != rbt->nil) {
        if ((*root)->left != rbt->nil) {
            rb_tree_destroy(rbt, &(*root)->left);
        }

        if ((*root)->right != rbt->nil) {
            rb_tree_destroy(rbt, &(*root)->right);
        }

        rbnode_destroy(&(*root), connection_destroy);
        *root = NULL;
    }
}

redblack_t *redblack_create(void) {
    redblack_t *rbt = (redblack_t *) malloc(sizeof(redblack_t));
    if (rbt == NULL) {
        return rbt;
    }

    rbt->root = rbt->nil = rbnode_create(-1, NULL);
    rbt->nil->parent = rbt->nil->right = rbt->nil->left = rbt->nil;
    rbt->nil->color = BLACK;
    rbt->size = 0;
    return rbt;
}

void redblack_destroy(redblack_t **rbt) {
    if (rbt != NULL && *rbt != NULL) {
        rb_tree_destroy(*rbt, &(*rbt)->root);

        rbnode_destroy(&(*rbt)->nil, connection_destroy);
        free(*rbt);
        *rbt = NULL;
    }
}

void redblack_insert(redblack_t *rbt, int key, connection_t *val) {
    rb_insert(rbt, key, val);
}

connection_t *redblack_extract(redblack_t *rbt, int key) {
    rbnode_t *n = rb_tree_search(rbt, rbt->root, key);
    if (n == rbt->nil) {
        return NULL;
    }

    connection_t *conn = n->val;
    rb_delete(rbt, n, NULL);
    rbt->size -= 1;
    return conn;
}

void redblack_delete(redblack_t *rbt, int key) {
    rbnode_t *n = rb_tree_search(rbt, rbt->root, key);
    if (n == rbt->nil) {
        return;
    }

    rb_delete(rbt, n, connection_destroy);
    rbt->size -= 1;
}
