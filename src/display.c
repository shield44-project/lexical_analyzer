#include "analyzer.h"
#include <stdio.h>

const char *token_type_name(TokenType type) {
    switch (type) {
        case TOK_KEYWORD: return "KEYWORD";
        case TOK_IDENTIFIER: return "IDENTIFIER";
        case TOK_NUMBER: return "NUMBER";
        case TOK_STRING: return "STRING";
        case TOK_CHAR: return "CHAR";
        case TOK_OPERATOR: return "OPERATOR";
        case TOK_SYMBOL: return "SYMBOL";
        case TOK_COMMENT: return "COMMENT";
        case TOK_PREPROCESSOR: return "PREPROCESSOR";
        case TOK_UNKNOWN: return "UNKNOWN";
        case TOK_END: return "END";
    }
    return "UNKNOWN";
}

static const char *token_color(TokenType type) {
    switch (type) {
        case TOK_KEYWORD: return BLUE;
        case TOK_IDENTIFIER: return CYAN;
        case TOK_NUMBER: return GREEN;
        case TOK_STRING:
        case TOK_CHAR: return YELLOW;
        case TOK_UNKNOWN: return RED;
        default: return RESET;
    }
}

void print_tokens(Token tokens[], int count) {
    printf("%-5s %-5s %-14s %s\n", "LINE", "COL", "TOKEN", "VALUE");
    printf("--------------------------------------------------\n");
    for (int i = 0; i < count; i++) {
        if (tokens[i].type == TOK_END) {
            continue;
        }
        printf("%-5d %-5d %s%-14s%s %s\n", tokens[i].line,
               tokens[i].col, token_color(tokens[i].type),
               token_type_name(tokens[i].type), RESET, tokens[i].text);
    }
}
