#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"

// Función dedicada para la limpieza de memoria del Token
void free_token(Token *t) {
    switch (t->type) {
        case TOKEN_START_TAG:
            if (t->data.start_tag.tag_name != NULL) {
                free(t->data.start_tag.tag_name);
                t->data.start_tag.tag_name = NULL;
            }
            // Nota: Aquí liberarás tu variable auxiliar de atributos en el futuro
            break;
            
        case TOKEN_END_TAG:
            if (t->data.end_tag.tag_name != NULL) {
                free(t->data.end_tag.tag_name);
                t->data.end_tag.tag_name = NULL;
            }
            break;
            
        case TOKEN_TEXT:
            if (t->data.text.content != NULL) {
                free(t->data.text.content);
                t->data.text.content = NULL;
            }
            break;
            
        default:
            // TOKEN_EOF y TOKEN_ERROR no tienen memoria dinámica asignada
            break;
    }
}

int main() {
    // El caso de prueba sugerido
    const char *input = "<div>Hola</div>";

    // Inicialización de la máquina de estados
    Tokenizer tok;
    tok.input = input;
    tok.position = 0;
    tok.length = strlen(input);
    tok.state = STATE_DATA;

    printf("Iniciando prueba con input: \"%s\"\n", tok.input);
    printf("Longitud: %zu bytes\n", tok.length);
    printf("----------------------------------------\n");

    // Bucle principal de lectura
    while (1) {
        Token t = get_next_token(&tok);

        switch (t.type) {
            case TOKEN_START_TAG:
                printf("[TOKEN_START_TAG] tag_name: \"%s\"\n", t.data.start_tag.tag_name);
                break;
                
            case TOKEN_END_TAG:
                printf("[TOKEN_END_TAG]   tag_name: \"%s\"\n", t.data.end_tag.tag_name);
                break;
                
            case TOKEN_TEXT:
                printf("[TOKEN_TEXT]      content:  \"%s\"\n", t.data.text.content);
                break;
                
            case TOKEN_EOF:
                printf("[TOKEN_EOF]       Lectura finalizada correctamente.\n");
                break;
                
            case TOKEN_ERROR:
                printf("[TOKEN_ERROR]     Fallo del tokenizer en la posicion %zu.\n", tok.position);
                break;
                
            default:
                printf("[TOKEN DESCONOCIDO] Tipo de token (%d) no manejado.\n", t.type);
                break;
        }

        // Liberar la memoria dinámica del token extraído
        free_token(&t);

        // Romper el bucle infinito si llegamos al final o a un error
        if (t.type == TOKEN_EOF || t.type == TOKEN_ERROR) {
            break;
        }
    }

    printf("----------------------------------------\n");
    return 0;
}