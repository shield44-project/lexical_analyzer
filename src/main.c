#include "analyzer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REPORT_NAME_SIZE 160

typedef enum {
    VIEW_ALL = 0,
    VIEW_KEYWORD,
    VIEW_IDENTIFIER,
    VIEW_NUMBER,
    VIEW_STRING,
    VIEW_CHAR,
    VIEW_OPERATOR,
    VIEW_SYMBOL,
    VIEW_COMMENT,
    VIEW_PREPROCESSOR,
    VIEW_UNKNOWN
} TokenViewMode;

static const char *view_mode_name(TokenViewMode mode) {
    switch (mode) {
        case VIEW_ALL: return "ALL TOKENS";
        case VIEW_KEYWORD: return "KEYWORDS";
        case VIEW_IDENTIFIER: return "IDENTIFIERS";
        case VIEW_NUMBER: return "NUMBERS";
        case VIEW_STRING: return "STRINGS";
        case VIEW_CHAR: return "CHARS";
        case VIEW_OPERATOR: return "OPERATORS";
        case VIEW_SYMBOL: return "SYMBOLS";
        case VIEW_COMMENT: return "COMMENTS";
        case VIEW_PREPROCESSOR: return "PREPROCESSORS";
        case VIEW_UNKNOWN: return "UNKNOWN TOKENS";
    }
    return "ALL TOKENS";
}

static int token_matches_view(TokenType type, TokenViewMode mode) {
    if (mode == VIEW_ALL) return 1;
    if (mode == VIEW_KEYWORD) return type == TOK_KEYWORD;
    if (mode == VIEW_IDENTIFIER) return type == TOK_IDENTIFIER;
    if (mode == VIEW_NUMBER) return type == TOK_NUMBER;
    if (mode == VIEW_STRING) return type == TOK_STRING;
    if (mode == VIEW_CHAR) return type == TOK_CHAR;
    if (mode == VIEW_OPERATOR) return type == TOK_OPERATOR;
    if (mode == VIEW_SYMBOL) return type == TOK_SYMBOL;
    if (mode == VIEW_COMMENT) return type == TOK_COMMENT;
    if (mode == VIEW_PREPROCESSOR) return type == TOK_PREPROCESSOR;
    if (mode == VIEW_UNKNOWN) return type == TOK_UNKNOWN;
    return 1;
}

static int prompt_yes_no(const char *prompt) {
    char answer[16];
    printf("%s", prompt);
    if (!fgets(answer, sizeof(answer), stdin)) return 0;
    return answer[0] == 'y' || answer[0] == 'Y';
}

static int env_truthy(const char *name) {
    const char *value = getenv(name);
    return value && value[0] && value[0] != '0';
}

static int prompt_choice(const char *title, const char *const options[], int count) {
    char answer[32];
    long choice = 0;
    char *end;

    printf("\n%s\n", title);
    for (int i = 0; i < count; i++) {
        printf("  %d) %s\n", i + 1, options[i]);
    }
    printf("Enter choice [1-%d]: ", count);
    if (!fgets(answer, sizeof(answer), stdin)) return 1;
    choice = strtol(answer, &end, 10);
    if (end == answer || choice < 1 || choice > count) return 1;
    return (int)choice;
}

static TokenViewMode prompt_token_view(void) {
    const char *options[] = {
        "Show all tokens",
        "Show only keywords",
        "Show only identifiers",
        "Show only numbers",
        "Show only strings",
        "Show only chars",
        "Show only operators",
        "Show only symbols",
        "Show only comments",
        "Show only preprocessor lines",
        "Show only unknown tokens"
    };
    int choice = prompt_choice("Which tokens should be displayed?", options, 11);
    switch (choice) {
        case 2: return VIEW_KEYWORD;
        case 3: return VIEW_IDENTIFIER;
        case 4: return VIEW_NUMBER;
        case 5: return VIEW_STRING;
        case 6: return VIEW_CHAR;
        case 7: return VIEW_OPERATOR;
        case 8: return VIEW_SYMBOL;
        case 9: return VIEW_COMMENT;
        case 10: return VIEW_PREPROCESSOR;
        case 11: return VIEW_UNKNOWN;
        default: return VIEW_ALL;
    }
}

static int write_filtered_tokens(FILE *out, Token tokens[], int count, TokenViewMode mode, int colors) {
    Token filtered[MAX_TOKENS];
    int visible = 0;
    for (int i = 0; i < count && visible < MAX_TOKENS - 1; i++) {
        if (tokens[i].type == TOK_END) continue;
        if (!token_matches_view(tokens[i].type, mode)) continue;
        filtered[visible++] = tokens[i];
    }
    filtered[visible].type = TOK_END;
    snprintf(filtered[visible].text, sizeof(filtered[visible].text), "END");
    filtered[visible].line = tokens[count - 1].line;
    filtered[visible].col = tokens[count - 1].col;
    fprintf(out, "Token view: %s\n", view_mode_name(mode));
    print_tokens_report(out, filtered, visible + 1, colors);
    return visible;
}

static int syntax_error_count(Token tokens[], int count) {
    FILE *sink = tmpfile();
    int errors;
    if (!sink) return syntax_analyze_report(stdout, tokens, count, 0);
    errors = syntax_analyze_report(sink, tokens, count, 0);
    fclose(sink);
    return errors;
}
static char *read_file(const char *path) {
    FILE *fp = fopen(path, "rb");
    long size;
    char *data;
    if (!fp) {
        perror(path);
        return NULL;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }
    size = ftell(fp);
    rewind(fp);
    data = malloc((size_t)size + 1);
    if (!data) {
        fclose(fp);
        return NULL;
    }
    if (fread(data, 1, (size_t)size, fp) != (size_t)size) {
        free(data);
        fclose(fp);
        return NULL;
    }
    data[size] = '\0';
    fclose(fp);
    return data;
}
static void report_name(const char *path, char out[], size_t size) {
    const char *name = strrchr(path, '/');
    size_t used = 0;
    name = name ? name + 1 : path;
    for (size_t i = 0; name[i] && name[i] != '.' && used + 1 < size; i++) {
        unsigned char ch = (unsigned char)name[i];
        out[used++] = (char)(isalnum(ch) ? ch : '_');
    }
    if (used == 0 && size > 1) out[used++] = 'c';
    out[used] = '\0';
    snprintf(out + used, size - used, "_run.txt");
}
static int write_report(FILE *out, Token tokens[], int count, int colors,
                        TokenViewMode mode, int show_syntax) {
    int errors;
    int visible = write_filtered_tokens(out, tokens, count, mode, colors);
    if (show_syntax) {
        errors = syntax_analyze_report(out, tokens, count, colors);
    } else {
        errors = syntax_error_count(tokens, count);
    }
    fprintf(out, "\n%sDisplayed tokens:%s %d of %d\n",
            colors ? BLUE : "", colors ? RESET : "", visible, count - 1);
    fprintf(out, "%sTotal tokens:%s %d\n", colors ? BLUE : "", colors ? RESET : "", count - 1);
    fprintf(out, "%sSyntax errors:%s %d\n", errors ? (colors ? RED : "") :
            (colors ? GREEN : ""), colors ? RESET : "", errors);
    return errors;
}
static FILE *open_report_if_requested(const char *source_path, char report_path[], int wanted) {
    FILE *report;
    if (!wanted) return NULL;
    report_name(source_path, report_path, REPORT_NAME_SIZE);
    report = fopen(report_path, "w");
    if (!report) perror(report_path);
    return report;
}
int main(int argc, char *argv[]) {
    char report_path[REPORT_NAME_SIZE];
    char *source;
    FILE *report;
    Token tokens[MAX_TOKENS];
    int count, errors, save_report, show_syntax;
    TokenViewMode view_mode;
    int noninteractive = env_truthy("ANALYZER_NONINTERACTIVE");
    if (argc != 2) {
        printf("Usage: %s file.c\n", argv[0]);
        return 1;
    }
    source = read_file(argv[1]);
    if (!source) return 1;
    loading_animation("Analyzing source code...", 800);
    count = lexical_analyze(source, tokens, MAX_TOKENS);
    print_banner(stdout, 1);
    if (noninteractive) {
        view_mode = VIEW_ALL;
        show_syntax = 1;
        save_report = 0;
    } else {
        view_mode = prompt_token_view();
        show_syntax = prompt_yes_no("Show syntax analysis section? (yes/no): ");
        save_report = prompt_yes_no("Store this analyser output in a text file? (yes/no): ");
    }
    report = open_report_if_requested(argv[1], report_path, save_report);
    errors = write_report(stdout, tokens, count, 1, view_mode, show_syntax);
    if (report) {
        print_banner(report, 0);
        write_report(report, tokens, count, 0, view_mode, show_syntax);
        fclose(report);
        printf("\nSaved analyser output to %s\n", report_path);
    }
    free(source);
    return errors ? 1 : 0;
}
