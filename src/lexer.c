#include "analyzer.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static void add_token(Token t[], int *n, TokenType type, const char *start,
                      int len, int line, int col) {
    int copy = len < MAX_LEXEME - 1 ? len : MAX_LEXEME - 1;
    t[*n].type = type;
    memcpy(t[*n].text, start, (size_t)copy);
    t[*n].text[copy] = '\0';
    t[*n].line = line;
    t[*n].col = col;
    (*n)++;
}

static void move_one(const char *s, int *i, int *line, int *col) {
    if (s[*i] == '\n') {
        (*line)++;
        *col = 1;
    } else {
        (*col)++;
    }
    (*i)++;
}

static int two_char_operator(const char *s) {
    const char *ops[] = {
        "==","!=","<=",">=","++","--","+=","-=","*=","/=",
        "&&","||","->", NULL
    };
    for (int i = 0; ops[i]; i++) {
        if (strncmp(s, ops[i], 2) == 0) return 1;
    }
    return 0;
}

int lexical_analyze(const char *s, Token t[], int max_tokens) {
    int i = 0, line = 1, col = 1, n = 0;
    while (s[i] && n < max_tokens - 1) {
        int start = i, start_line = line, start_col = col;
        char c = s[i];
        if (isspace((unsigned char)c)) {
            move_one(s, &i, &line, &col);
        } else if (c == '#') {
            while (s[i] && s[i] != '\n') move_one(s, &i, &line, &col);
            add_token(t, &n, TOK_PREPROCESSOR, s + start, i - start,
                      start_line, start_col);
        } else if (isalpha((unsigned char)c) || c == '_') {
            char word[MAX_LEXEME];
            while (isalnum((unsigned char)s[i]) || s[i] == '_') {
                move_one(s, &i, &line, &col);
            }
            snprintf(word, sizeof(word), "%.*s", i - start, s + start);
            add_token(t, &n, is_keyword(word) ? TOK_KEYWORD : TOK_IDENTIFIER,
                      s + start, i - start, start_line, start_col);
        } else if (isdigit((unsigned char)c)) {
            while (isalnum((unsigned char)s[i]) || s[i] == '.') {
                move_one(s, &i, &line, &col);
            }
            add_token(t, &n, TOK_NUMBER, s + start, i - start,
                      start_line, start_col);
        } else if (c == '"' || c == '\'') {
            char quote = c;
            move_one(s, &i, &line, &col);
            while (s[i] && s[i] != quote) {
                if (s[i] == '\\' && s[i + 1]) move_one(s, &i, &line, &col);
                move_one(s, &i, &line, &col);
            }
            if (s[i] == quote) move_one(s, &i, &line, &col);
            add_token(t, &n, quote == '"' ? TOK_STRING : TOK_CHAR,
                      s + start, i - start, start_line, start_col);
        } else if (c == '/' && (s[i + 1] == '/' || s[i + 1] == '*')) {
            int block = s[i + 1] == '*';
            move_one(s, &i, &line, &col);
            move_one(s, &i, &line, &col);
            while (s[i] && ((block && !(s[i] == '*' && s[i + 1] == '/')) ||
                   (!block && s[i] != '\n'))) move_one(s, &i, &line, &col);
            if (block && s[i]) { move_one(s, &i, &line, &col);
                move_one(s, &i, &line, &col); }
            add_token(t, &n, TOK_COMMENT, s + start, i - start,
                      start_line, start_col);
        } else if (strchr("+-*/%=!<>&|", c)) {
            int len = two_char_operator(s + i) ? 2 : 1;
            while (len--) move_one(s, &i, &line, &col);
            add_token(t, &n, TOK_OPERATOR, s + start, i - start,
                      start_line, start_col);
        } else if (strchr("(){}[];,:.", c)) {
            move_one(s, &i, &line, &col);
            add_token(t, &n, TOK_SYMBOL, s + start, 1, start_line, start_col);
        } else {
            move_one(s, &i, &line, &col);
            add_token(t, &n, TOK_UNKNOWN, s + start, 1, start_line, start_col);
        }
    }
    add_token(t, &n, TOK_END, "END", 3, line, col);
    return n;
}
