#include "analyzer.h"
#include <stdio.h>
#include <string.h>

static int ignored(TokenType type) {
    return type == TOK_COMMENT || type == TOK_PREPROCESSOR;
}

static int is_type_word(const char *text) {
    const char *types[] = {
        "int","float","char","double","void","long","short",
        "unsigned","signed","struct","const","static", NULL
    };
    for (int i = 0; types[i]; i++) {
        if (strcmp(text, types[i]) == 0) return 1;
    }
    return 0;
}

static int next_real(Token t[], int count, int i) {
    i++;
    while (i < count && ignored(t[i].type)) i++;
    return i;
}

static int prev_real(Token t[], int i) {
    i--;
    while (i >= 0 && ignored(t[i].type)) i--;
    return i;
}
static int find_end(Token t[], int count, int i) {
    int start_line = t[i].line;
    while (i < count && t[i].type != TOK_END) {
        if (strcmp(t[i].text, ";") == 0) return 1;
        if (strcmp(t[i].text, "{") == 0) return 1;
        if (strcmp(t[i].text, "}") == 0) return 0;
        if (t[i].line > start_line && t[i].type == TOK_KEYWORD) return 0;
        i++;
    }
    return 0;
}

static void syntax_error(Token token, const char *msg, int *errors) {
    printf(RED "Syntax error at line %d, col %d: %s\n" RESET,
           token.line, token.col, msg);
    (*errors)++;
}

static char matching(char c) {
    if (c == ')') return '(';
    if (c == '}') return '{';
    if (c == ']') return '[';
    return '?';
}

int syntax_analyze(Token t[], int count) {
    char stack[200];
    int top = 0, errors = 0;
    printf("\nSyntax Analysis\n");
    printf("---------------\n");
    for (int i = 0; i < count && t[i].type != TOK_END; i++) {
        int n = next_real(t, count, i);
        int p = prev_real(t, i);
        if (ignored(t[i].type)) continue;
        if (t[i].type == TOK_UNKNOWN) syntax_error(t[i], "unknown symbol", &errors);
        if (strchr("({[", t[i].text[0]) && t[i].text[1] == '\0') {
            if (top < 200) stack[top++] = t[i].text[0];
        }
        if (strchr(")}]", t[i].text[0]) && t[i].text[1] == '\0') {
            if (top == 0 || stack[top - 1] != matching(t[i].text[0]))
                syntax_error(t[i], "unmatched closing bracket", &errors);
            else top--;
        }
        if ((strcmp(t[i].text, "if") == 0 || strcmp(t[i].text, "for") == 0 ||
             strcmp(t[i].text, "while") == 0 || strcmp(t[i].text, "switch") == 0) &&
            n < count && strcmp(t[n].text, "(") != 0) {
            syntax_error(t[i], "expected '(' after control keyword", &errors);
        }
        if ((strcmp(t[i].text, "return") == 0 || strcmp(t[i].text, "break") == 0 ||
             strcmp(t[i].text, "continue") == 0) && !find_end(t, count, i)) {
            syntax_error(t[i], "missing semicolon", &errors);
        }
        if (t[i].type == TOK_KEYWORD && is_type_word(t[i].text) &&
            !find_end(t, count, i)) {
            syntax_error(t[i], "missing semicolon in declaration", &errors);
        }
        if (t[i].type == TOK_IDENTIFIER && n < count &&
            (strcmp(t[n].text, "=") == 0 || strcmp(t[n].text, "(") == 0) &&
            !(p >= 0 && t[p].type == TOK_KEYWORD && is_type_word(t[p].text)) &&
            !find_end(t, count, i)) {
            syntax_error(t[i], "missing semicolon", &errors);
        }
    }
    if (top > 0) {
        printf(RED "Syntax error: some opening brackets are not closed\n" RESET);
        errors++;
    }
    if (errors == 0) printf(GREEN "No syntax errors found.\n" RESET);
    return errors;
}
