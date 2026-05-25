#ifndef ANALYZER_H
#define ANALYZER_H

#define MAX_TOKENS 2000
#define MAX_LEXEME 80

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"
#define RESET "\033[0m"

typedef enum {
    TOK_KEYWORD,
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_STRING,
    TOK_CHAR,
    TOK_OPERATOR,
    TOK_SYMBOL,
    TOK_COMMENT,
    TOK_PREPROCESSOR,
    TOK_UNKNOWN,
    TOK_END
} TokenType;

typedef struct {
    TokenType type;
    char text[MAX_LEXEME];
    int line;
    int col;
} Token;

int lexical_analyze(const char *source, Token tokens[], int max_tokens);
int syntax_analyze(Token tokens[], int count);
void print_tokens(Token tokens[], int count);
const char *token_type_name(TokenType type);
int is_keyword(const char *word);

#endif
