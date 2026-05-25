#include "analyzer.h"
#include <stdio.h>
#include <stdlib.h>

static char *read_file(const char *path) {
    FILE *fp = fopen(path, "rb");
    long size;
    char *data;

    if (!fp) {
        perror(path);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);
    data = malloc((size_t)size + 1);
    if (!data) {
        fclose(fp);
        return NULL;
    }
    fread(data, 1, (size_t)size, fp);
    data[size] = '\0';
    fclose(fp);
    return data;
}

int main(int argc, char *argv[]) {
    char *source;
    Token tokens[MAX_TOKENS];
    int count;
    int errors;

    if (argc != 2) {
        printf("Usage: %s file.c\n", argv[0]);
        return 1;
    }
    source = read_file(argv[1]);
    if (!source) {
        return 1;
    }
    printf(CYAN "Lexical Analyzer and Syntax Analyzer\n" RESET);
    printf("------------------------------------\n\n");
    count = lexical_analyze(source, tokens, MAX_TOKENS);
    print_tokens(tokens, count);
    errors = syntax_analyze(tokens, count);
    printf("\n%sTotal tokens:%s %d\n", BLUE, RESET, count - 1);
    printf("%sSyntax errors:%s %d\n", errors ? RED : GREEN, RESET, errors);
    free(source);
    return errors ? 1 : 0;
}
