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
                if (tok->input[tok->position + 1] == '\0') {
                    
                    tok->state = STATE_EOF;
                    return token_nulo;
                }
                determinator = 1;
                switch (tok->input[tok->position + 1]) {
                    case '/':{
                        tok->state = STATE_END_TAG_OPEN;
                        tok->position += 2;
                        break;
                    }
                    case '!':{
                        char *end_tag = strstr(tok->input + tok->position, ">");
                        if (end_tag == NULL) {
                            tok->state = STATE_EOF;
                            return token_nulo;
                        }
                        position = end_tag - tok->input;
                        tok->position = position + 1;
                        tok->state = STATE_DATA;
                        break;
                    }
                    default:{
                        tok->state = STATE_TAG_NAME;
                        tok->position += 1;
                        break;
                    }
                }
                break;
            }

            case STATE_TAG_NAME:{
                int tag_name_log = strcspn(tok->input + tok->position, " />");
                char *tag_name = malloc(tag_name_log + 1);
                if (tag_name == NULL) {
                    printf("Error en state_tag_name\n");
                    return token_nulo;
                }
                memcpy(tag_name, tok->input + tok->position, tag_name_log);
                tag_name[tag_name_log] = '\0';
                switch (tok->input[tok->position + tag_name_log]) {
                    case ' ':{
                        determinator = 1;
                        token.type = TOKEN_START_TAG;
                        tok->state = STATE_BEFORE_ATTRIBUTE_NAME;
                        tok->position += (tag_name_log + 1);
                        token.data.start_tag.tag_name = tag_name;
                        token.data.start_tag.attributes = NULL;
                        token.data.start_tag.attributes_capacity = 0;
                        token.data.start_tag.attributes_count = 0;
                        break;
                    }
                    case '/':{
                        tok->state = STATE_SELF_CLOSING_TAG;
                        token.type = TOKEN_START_TAG;
                        determinator = 1;
                        token.data.start_tag.tag_name = tag_name;
                        tok->position += tag_name_log + 1;
                        break;
                    }
                    case '>':{
                        token.type = TOKEN_START_TAG;
                        tok->state = STATE_DATA;
                        token.data.start_tag.tag_name = tag_name;
                        determinator = 0; 
                        tok->position += tag_name_log + 1;
                        break;
                    }
                    default:{
                        printf("Error de html\n");
                        return token_nulo;
                    }
                }
                break;
            }

            case STATE_END_TAG_OPEN:{
                char *end_tag = strstr(tok->input + tok->position, ">");
                if (end_tag == NULL) {
                    tok->state = STATE_EOF;
                    return token_nulo;
                }
                int log = end_tag - (tok->input + tok->position);

                char *text = malloc(log + 1);
                if (text == NULL) {
                    printf("Error de malloc en STATE_END_TAG_OPEN\n");
                    return token_nulo;
                }
                memcpy(text, tok->input + tok->position, log);
                text[log] = '\0';

                token.type = TOKEN_END_TAG;
                token.data.end_tag.tag_name = text;

                tok->position += log + 1;
                tok->state = STATE_DATA;
                determinator = 0;
                break;
            }

            case STATE_BEFORE_ATTRIBUTE_NAME:{
                switch (tok->input[tok->position]) {
                    case '>':{
                        tok->state = STATE_DATA;
                        determinator = 0; 
                        tok->position ++;
                        break;
                    }
                    case '/':{
                        
                    }
                    case ' ':{

                    }
                    default :{

                    }
                }
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