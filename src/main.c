#include "analyzer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REPORT_NAME_SIZE 160
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
static int wants_report(void) {
    char answer[16];
    printf("Store this analyser output in a text file? (yes/no): ");
    if (!fgets(answer, sizeof(answer), stdin)) return 0;
    return answer[0] == 'y' || answer[0] == 'Y';
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
static int write_report(FILE *out, Token tokens[], int count, int colors) {
    int errors;
    print_tokens_report(out, tokens, count, colors);
    errors = syntax_analyze_report(out, tokens, count, colors);
    fprintf(out, "\n%sTotal tokens:%s %d\n", colors ? BLUE : "", colors ? RESET : "", count - 1);
    fprintf(out, "%sSyntax errors:%s %d\n", errors ? (colors ? RED : "") :
            (colors ? GREEN : ""), colors ? RESET : "", errors);
    return errors;
}
static FILE *open_report_if_requested(const char *source_path, char report_path[]) {
    FILE *report;
    if (!wants_report()) return NULL;
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
    int count, errors;
    if (argc != 2) {
        printf("Usage: %s file.c\n", argv[0]);
        return 1;
    }
    source = read_file(argv[1]);
    if (!source) return 1;
    count = lexical_analyze(source, tokens, MAX_TOKENS);
    print_banner(stdout, 1);
    report = open_report_if_requested(argv[1], report_path);
    errors = write_report(stdout, tokens, count, 1);
    if (report) {
        print_banner(report, 0);
        write_report(report, tokens, count, 0);
        fclose(report);
        printf("\nSaved analyser output to %s\n", report_path);
    }
    free(source);
    return errors ? 1 : 0;
}
