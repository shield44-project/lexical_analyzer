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
        if (strcmp(t[i].text, ";") == 0 || strcmp(t[i].text, "{") == 0) return 1;
        if (strcmp(t[i].text, "}") == 0) return 0;
        if (t[i].line > start_line && t[i].type == TOK_KEYWORD) return 0;
        i++;
    }
    return 0;
}
static void syntax_error(FILE *out, Token token, const char *msg,
                         int *errors, int colors) {
    fprintf(out, "%sSyntax error at line %d, col %d: %s%s\n",
            colors ? RED : "", token.line, token.col, msg, colors ? RESET : "");
    (*errors)++;
}
static char matching(char c) {
    if (c == ')') return '(';
    if (c == '}') return '{';
    if (c == ']') return '[';
    return '?';
}
static int is_control_word(const char *text) {
    return strcmp(text, "if") == 0 || strcmp(text, "for") == 0 ||
           strcmp(text, "while") == 0 || strcmp(text, "switch") == 0;
}
static int is_jump_word(const char *text) {
    return strcmp(text, "return") == 0 || strcmp(text, "break") == 0 ||
           strcmp(text, "continue") == 0;
}
static void check_token(FILE *out, Token t[], int count, int i,
                        char stack[], int *top, int *errors, int colors) {
    int n = next_real(t, count, i), p = prev_real(t, i);
    if (t[i].type == TOK_UNKNOWN) syntax_error(out, t[i], "unknown symbol", errors, colors);
    if (strchr("({[", t[i].text[0]) && t[i].text[1] == '\0' && *top < 200)
        stack[(*top)++] = t[i].text[0];
    if (strchr(")}]", t[i].text[0]) && t[i].text[1] == '\0') {
        if (*top == 0 || stack[*top - 1] != matching(t[i].text[0]))
            syntax_error(out, t[i], "unmatched closing bracket", errors, colors);
        else (*top)--;
    }
    if (is_control_word(t[i].text) && n < count && strcmp(t[n].text, "(") != 0)
        syntax_error(out, t[i], "expected '(' after control keyword", errors, colors);
    if (is_jump_word(t[i].text) && !find_end(t, count, i))
        syntax_error(out, t[i], "missing semicolon", errors, colors);
    if (t[i].type == TOK_KEYWORD && is_type_word(t[i].text) && !find_end(t, count, i))
        syntax_error(out, t[i], "missing semicolon in declaration", errors, colors);
    if (t[i].type == TOK_IDENTIFIER && n < count &&
        (strcmp(t[n].text, "=") == 0 || strcmp(t[n].text, "(") == 0) &&
        !(p >= 0 && t[p].type == TOK_KEYWORD && is_type_word(t[p].text)) &&
        !find_end(t, count, i))
        syntax_error(out, t[i], "missing semicolon", errors, colors);
}
int syntax_analyze_report(FILE *out, Token t[], int count, int colors) {
    char stack[200];
    int top = 0, errors = 0;
    fprintf(out, "\nSyntax Analysis\n---------------\n");
    for (int i = 0; i < count && t[i].type != TOK_END; i++) {
        if (!ignored(t[i].type)) check_token(out, t, count, i, stack, &top, &errors, colors);
    }
    if (top > 0) {
        fprintf(out, "%sSyntax error: some opening brackets are not closed%s\n",
                colors ? RED : "", colors ? RESET : "");
        errors++;
    }
    if (errors == 0)
        fprintf(out, "%sNo syntax errors found.%s\n", colors ? GREEN : "", colors ? RESET : "");
    return errors;
}
int syntax_analyze(Token tokens[], int count) {
    return syntax_analyze_report(stdout, tokens, count, 1);
}
