#ifndef NODE_H
#define NODE_H

#include <stddef.h>

typedef enum{
    NODE_ELEMENT,
    NODE_TEXT,
    NODE_COMMENT
} NodeType;

typedef union{
    struct{
        char *tag_name;
    }element;

    struct{
        char *content;
    }text;

    struct{
        char *content;
    }comment;

} NodeData;

typedef struct Node{
    NodeType type;
    NodeData data;
    struct Node *parent;
    struct Node *first_child;
    struct Node *last_child;
    struct Node *next_sibling;
    struct Node *prev_sibling;
} Node;

Node *create_text_content(const char *content);
Node *create_element_node(const char *tag_name);
Node *create_node_comment(const char *content);
void append_child(Node *parent, Node *child);
void free_node_tree (Node *node);

#endif