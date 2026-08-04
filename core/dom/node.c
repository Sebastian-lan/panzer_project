#include "node.h"
#include <openssl/core_dispatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

Node *create_text_content(const char *content){
    Node *node = malloc(sizeof(Node));
    if (node == NULL) {
        printf("Error en create_text_content #: %d\n", 1);
        return NULL;
    }
    node->type = NODE_TEXT;
    
    node->data.text.content = malloc(strlen(content) + 1);
    
    if (node->data.text.content == NULL) {
        free(node);
        printf("Error en create_text_content #: %d\n",2);
        return NULL;
    }
    strcpy(node->data.text.content, content);

    node->first_child = NULL;
    node->last_child = NULL;
    node->next_sibling = NULL;
    node->prev_sibling = NULL;
    node->parent = NULL;

    return node;
};

Node *create_element_node(const char *tag_name){
    
    Node *node = malloc(sizeof(Node));
    if (node == NULL) {
        printf("Error en create_element_node #: %d\n", 1);
        return NULL;
    }

    node->type = NODE_ELEMENT;

    node->data.element.tag_name = malloc(strlen(tag_name) + 1);
    if (node->data.element.tag_name == NULL) {
        free(node);
        printf("Error en create_element_node #: %d\n", 2);
        return NULL;
    }

    strcpy(node->data.element.tag_name, tag_name);

    node->first_child = NULL;
    node->last_child = NULL;
    node->next_sibling = NULL;
    node->prev_sibling = NULL;
    node->parent = NULL;

    return node;
}

Node *create_node_comment(const char *content){

    Node *node = malloc(sizeof(Node));
    if (node == NULL) {
        printf("Error en create_node_comment #: %d\n", 1);
        return NULL;
    }
    
    node->type = NODE_COMMENT;

    node->data.comment.content = malloc(strlen(content) + 1);
    if (node->data.comment.content == NULL) {
        free(node);
        printf("Error en create_node_comment #: %d\n", 2);
        return NULL;
    }
    strcpy(node->data.comment.content, content);

    node->first_child = NULL;
    node->last_child = NULL;
    node->next_sibling = NULL;
    node->prev_sibling = NULL;
    node->parent = NULL;

    return node;
}
void free_node_tree(Node *node){

    if (node == NULL) {
        return;
    }

    Node *child = node->first_child;
    while (child != NULL) {
        Node *next = child->next_sibling;
        free_node_tree(child);
        child = next;
    }

    switch (node->type) {
        case NODE_ELEMENT:{
            free(node->data.element.tag_name);
            break;
        }
        case NODE_TEXT: {
            free(node->data.text.content);
            break;
        }
        case NODE_COMMENT:{
            free(node->data.comment.content);
            break;
        }
    }

    free(node);
}

void append_child(Node *parent, Node *child){

    if (parent == NULL || child == NULL) {
        printf("Error al enlazar un hijo al nodo por valor null ingresado\n");
        return;
    }

    if (parent->first_child == NULL) {
        parent->first_child = child;
        parent->last_child = child;
    }else {
        child->prev_sibling = parent->last_child;
        parent->last_child->next_sibling = child;
        parent->last_child = child;
    }

    child->parent = parent;
}