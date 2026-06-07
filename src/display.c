#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "analyzer.h"
#include <stdio.h>
#include <unistd.h>

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

static const char *token_color(TokenType type, int colors) {
    if (!colors) return "";
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

static const char *reset_color(int colors) {
    return colors ? RESET : "";
}

void print_banner(FILE *out, int colors) {
    FILE *pipe = popen("figlet \"Lexical and Syntax Analyser\" 2>/dev/null", "r");
    char line[512];
    int wrote = 0;

    (void)colors;
    if (!pipe) return;
    while (fgets(line, sizeof(line), pipe)) {
        fputs(line, out);
        wrote = 1;
    }
    pclose(pipe);
    if (wrote) fprintf(out, "\n");
}

void print_tokens_report(FILE *out, Token tokens[], int count, int colors) {
    fprintf(out, "%-5s %-5s %-14s %s\n", "LINE", "COL", "TOKEN", "VALUE");
    fprintf(out, "--------------------------------------------------\n");
    for (int i = 0; i < count; i++) {
        if (tokens[i].type == TOK_END) continue;
        fprintf(out, "%-5d %-5d %s%-14s%s %s\n", tokens[i].line,
                tokens[i].col, token_color(tokens[i].type, colors),
                token_type_name(tokens[i].type), reset_color(colors),
                tokens[i].text);
    }
}

void print_tokens(Token tokens[], int count) {
    print_tokens_report(stdout, tokens, count, 1);
}

void loading_animation(const char *label, int duration_ms) {
    const char spinner[] = "|/-\\";
    int i = 0;
    int elapsed = 0;
    int step = 100; // ms

    printf("%s ", label);
    fflush(stdout);
    while (elapsed < duration_ms) {
        printf("\b%c", spinner[i % 4]);
        fflush(stdout);
        usleep(step * 1000);
        i++;
        elapsed += step;
    }
    printf("\bDone!\n");
}
