#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>
typedef enum {
    TOKEN_START_TAG,
    TOKEN_END_TAG,
    TOKEN_TEXT,
    TOKEN_COMMENT,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef enum{
    STATE_DATA,
    STATE_TAG_OPEN,
    STATE_END_TAG_OPEN,
    STATE_TAG_NAME,
    STATE_BEFORE_ATTRIBUTE_NAME,
    STATE_ATTRIBUTE_NAME,
    STATE_BEFORE_ATTRIBUTE_VALUE,
    STATE_ATTRIBUTE_VALUE_QUOTED,
    STATE_ATTRIBUTE_VALUE_UNQUOTED,
    STATE_MARKUP_DECLARATION,
    STATE_COMMENT,
    STATE_SELF_CLOSING_TAG,
    STATE_EOF
} TokenizerState;

typedef struct{
    char *name;
    char *value;
} Attribute;

typedef union{
    struct{
        char *tag_name;
        Attribute *attributes;
        size_t attributes_count;
        size_t attributes_capacity;
        int self_closing;
    }start_tag;
    
    struct{
        char *tag_name;
    }end_tag;

    struct{
        char *content;
    }text;

    struct{
        char *content;
    }comment;
} TokenData;
typedef struct{
    TokenType type;
    TokenData data;
} Token;

typedef struct{
    const char *input;
    size_t position;
    size_t length;
    TokenizerState state;
} Tokenizer;

Token get_next_token(Tokenizer *tok);

#endif