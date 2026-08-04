#include "node.h"
#include <stdio.h>
int main(){

    Node *html = create_element_node("html");
    Node *body = create_element_node("body");
    Node *div = create_element_node("div");
    Node *h1 = create_element_node("h1");
    Node *p = create_element_node("p");

    if (html == NULL) {
        printf("Error happens con node_el_1\n");
        return 0;
    }
    if (body == NULL) {
        printf("Error happens con node_el_2\n");
        return 0;
    }
    if (div == NULL) {
        printf("Error happens con node_el_3\n");
        return 0;
    }
    if (h1 == NULL) {
        printf("Error happens con node_el_4\n");
        return 0;
    }
    if (p == NULL) {
        printf("Error happens con node_el_5\n");
        return 0;
    }

    Node *Titulo = create_text_content("Titulo");
    Node *parrafo_prueba = create_text_content("Un parrafo de pruea");
    append_child(html, body);
    append_child(body, div);
    append_child(div, h1);
    append_child(div, p);
    append_child(h1, Titulo);
    append_child(p, parrafo_prueba);

    Node *node = html;
    while (node != NULL) {
        switch (node->type) {
            case NODE_ELEMENT:{
                printf("%s\n", node->data.element.tag_name);
                break;
            }
            case NODE_TEXT:{
                printf("%s\n", node->data.text.content);
                break;
            }
            case NODE_COMMENT:{
                printf("%s\n", node->data.comment.content);
                break;
            }
        }
        node = node->first_child;
    }

    free_node_tree(html);
    free_node_tree(node);
    return 0;

    
}