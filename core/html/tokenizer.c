#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Token get_next_token(Tokenizer *tok){
    Token token, token_nulo;
    token_nulo.type = TOKEN_ERROR;
    int determinator = 0;
    int position;
    do{
        switch (tok->state) {


            case STATE_DATA:{

                char *end_text = strstr(tok->input + tok->position, "<");

                if (end_text == NULL) {
                    int log = tok->length - tok->position;
                    determinator = 0;

                    if (log == 0) {
                        token.type = TOKEN_EOF;
                        break;

                    }else{

                        char *text = malloc(tok->length - tok->position + 1);

                        if (text == NULL) {
                            printf("Error de malloc en caso especial de get_next_token");
                            return token_nulo;
                        }

                        text[log] = '\0';
                        memcpy(text, tok->input + tok->position, log);
                        token.type = TOKEN_TEXT;
                        token.data.text.content = text;
                        tok->state = STATE_EOF;
                        tok->position = tok->length;
                        break;
                    }

                }else{

                    position = end_text - tok->input; 
                    tok->state = STATE_TAG_OPEN;
                }

                int longitud = position - tok->position;
                if (longitud == 0) {

                    determinator = 1;
                    break;
                }

                char *text = malloc(longitud+1);
                if (text == NULL) {

                    printf("Error en get_next_token #: %d\n", 1);
                    return token_nulo;
                }

                memcpy(text, tok->input + tok->position, longitud);
                text[longitud] = '\0';
                token.type = TOKEN_TEXT;
                token.data.text.content = text;
                determinator = 0;
                tok->position = position;
                break;

            }


            case STATE_TAG_OPEN:{
                
                break;
            }


            case STATE_EOF:{
                token.type = TOKEN_EOF;
                determinator = 0;
                break;
            }


            default:{
                return token_nulo;
            }
        }
    }while(determinator == 1);
    return token;
}