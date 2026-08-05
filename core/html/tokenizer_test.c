#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"

int main() {
    // 1. Hardcodeamos el input y la configuración inicial
    const char *input = "hola mundo";

    Tokenizer tok;
    tok.input = input;
    tok.position = 0;
    tok.length = strlen(input);
    tok.state = STATE_DATA;

    printf("Iniciando prueba con input: \"%s\"\n\n", tok.input);

    // 2. Usamos un bucle infinito que romperemos al encontrar EOF o ERROR
    while (1) {
        Token t = get_next_token(&tok);

        if (t.type == TOKEN_TEXT) {
            printf("[TOKEN_TEXT] Contenido: \"%s\"\n", t.data.text.content);
            printf("             -> Estado actual: %d, Posicion: %zu\n", tok.state, tok.position);
            
            // Liberamos la memoria del texto extraído
            free(t.data.text.content);
            
        } else if (t.type == TOKEN_EOF) {
            printf("[TOKEN_EOF] Lectura finalizada correctamente.\n");
            break; // Rompemos el bucle al terminar
            
        } else if (t.type == TOKEN_ERROR) {
            printf("[TOKEN_ERROR] Ocurrio un error en el tokenizer.\n");
            break; // Rompemos el bucle por seguridad
            
        } else {
            printf("[TOKEN DESCONOCIDO] Tipo: %d\n", t.type);
            break; 
        }
    }

    return 0;
}