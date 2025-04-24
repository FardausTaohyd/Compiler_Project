#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKENS 1000
#define MAX_TOKEN_LEN 50
#define MAX_VARS 100
#define MAX_INPUT 10000

// Token types
typedef enum {
    TOKEN_START, TOKEN_STOP, TOKEN_INT, TOKEN_FLOAT, TOKEN_IDENT,
    TOKEN_READ, TOKEN_WRITE, TOKEN_IF, TOKEN_ELSE, TOKEN_WHILE,
    TOKEN_NUMBER, TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULTIPLY, TOKEN_DIVIDE,
    TOKEN_ASSIGN, TOKEN_SEMICOLON, TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_LT, TOKEN_GT, TOKEN_EQ, TOKEN_LE, TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_EOF
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    char lexeme[MAX_TOKEN_LEN];
    float value; // For numbers
} Token;

// Variable structure
typedef struct {
    char name[MAX_TOKEN_LEN];
    int is_float;
    union {
        int i_val;
        float f_val;
    } value;
} Variable;

// Global variables
Token tokens[MAX_TOKENS];
int token_count = 0;
int current_token = 0;
Variable variables[MAX_VARS];
int var_count = 0;

// Lexer functions
void skip_whitespace(char **input) {
    while (isspace(**input)) (*input)++;
}

int is_keyword(char *str) {
    if (strcmp(str, "start") == 0) return TOKEN_START;
    if (strcmp(str, "stop") == 0) return TOKEN_STOP;
    if (strcmp(str, "int") == 0) return TOKEN_INT;
    if (strcmp(str, "float") == 0) return TOKEN_FLOAT;
    if (strcmp(str, "read") == 0) return TOKEN_READ;
    if (strcmp(str, "write") == 0) return TOKEN_WRITE;
    if (strcmp(str, "if") == 0) return TOKEN_IF;
    if (strcmp(str, "else") == 0) return TOKEN_ELSE;
    if (strcmp(str, "while") == 0) return TOKEN_WHILE;
    return -1;
}

void tokenize(char *input) {
    token_count = 0;
    while (*input) {
        skip_whitespace(&input);

        if (*input == '\0') break;

        Token *token = &tokens[token_count];

        if (isalpha(*input)) {
            char *start = input;
            while (isalnum(*input)) input++;
            strncpy(token->lexeme, start, input - start);
            token->lexeme[input - start] = '\0';

            int keyword = is_keyword(token->lexeme);
            if (keyword != -1) {
                token->type = keyword;
            } else {
                token->type = TOKEN_IDENT;
            }
        }
        else if (isdigit(*input) || *input == '.') {
            char *start = input;
            while (isdigit(*input) || *input == '.') input++;
            strncpy(token->lexeme, start, input - start);
            token->lexeme[input - start] = '\0';
            token->type = TOKEN_NUMBER;
            token->value = atof(token->lexeme);
        }
        else if (*input == '+') { token->type = TOKEN_PLUS; strcpy(token->lexeme, "+"); input++; }
        else if (*input == '-') { token->type = TOKEN_MINUS; strcpy(token->lexeme, "-"); input++; }
        else if (*input == '*') { token->type = TOKEN_MULTIPLY; strcpy(token->lexeme, "*"); input++; }
        else if (*input == '/') { token->type = TOKEN_DIVIDE; strcpy(token->lexeme, "/"); input++; }
        else if (*input == '=') {
            if (*(input+1) == '=') {
                token->type = TOKEN_EQ;
                strcpy(token->lexeme, "==");
                input += 2;
            } else {
                token->type = TOKEN_ASSIGN;
                strcpy(token->lexeme, "=");
                input++;
            }
        }
        else if (*input == ';') { token->type = TOKEN_SEMICOLON; strcpy(token->lexeme, ";"); input++; }
        else if (*input == '(') { token->type = TOKEN_LPAREN; strcpy(token->lexeme, "("); input++; }
        else if (*input == ')') { token->type = TOKEN_RPAREN; strcpy(token->lexeme, ")"); input++; }
        else if (*input == '<') {
            if (*(input+1) == '=') {
                token->type = TOKEN_LE;
                strcpy(token->lexeme, "<=");
                input += 2;
            } else {
                token->type = TOKEN_LT;
                strcpy(token->lexeme, "<");
                input++;
            }
        }
        else if (*input == '>') { token->type = TOKEN_GT; strcpy(token->lexeme, ">"); input++; }
        else if (*input == '{') { token->type = TOKEN_LBRACE; strcpy(token->lexeme, "{"); input++; }
        else if (*input == '}') { token->type = TOKEN_RBRACE; strcpy(token->lexeme, "}"); input++; }
        else {
            printf("Error: Invalid character '%c'\n", *input);
            exit(1);
        }

        token_count++;
    }
    tokens[token_count].type = TOKEN_EOF;
    strcpy(tokens[token_count].lexeme, "");
}

// Variable management
int find_variable(char *name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) return i;
    }
    return -1;
}

int add_variable(char *name, int is_float) {
    strcpy(variables[var_count].name, name);
    variables[var_count].is_float = is_float;
    return var_count++;
}

// Parser and interpreter
Token *next_token() {
    return &tokens[current_token++];
}

void expect(TokenType type) {
    if (tokens[current_token].type != type) {
        printf("Error: Expected %d, got %d\n", type, tokens[current_token].type);
        exit(1);
    }
    current_token++;
}

float expression();

float factor() {
    Token *token = next_token();
    if (token->type == TOKEN_NUMBER) {
        return token->value;
    }
    else if (token->type == TOKEN_IDENT) {
        int var_idx = find_variable(token->lexeme);
        if (var_idx == -1) {
            printf("Error: Undefined variable %s\n", token->lexeme);
            exit(1);
        }
        return variables[var_idx].is_float ?
               variables[var_idx].value.f_val :
               variables[var_idx].value.i_val;
    }
    else if (token->type == TOKEN_LPAREN) {
        float result = expression();
        expect(TOKEN_RPAREN);
        return result;
    }
    printf("Error: Invalid factor\n");
    exit(1);
}

float term() {
    float result = factor();
    while (tokens[current_token].type == TOKEN_MULTIPLY ||
           tokens[current_token].type == TOKEN_DIVIDE) {
        Token *op = next_token();
        float next = factor();
        if (op->type == TOKEN_MULTIPLY) result *= next;
        else result /= next;
    }
    return result;
}

float expression() {
    float result = term();
    while (tokens[current_token].type == TOKEN_PLUS ||
           tokens[current_token].type == TOKEN_MINUS) {
        Token *op = next_token();
        float next = term();
        if (op->type == TOKEN_PLUS) result += next;
        else result -= next;
    }
    return result;
}

int condition() {
    float left = expression();
    Token *op = next_token();
    float right = expression();

    if (op->type == TOKEN_LT) return left < right;
    if (op->type == TOKEN_GT) return left > right;
    if (op->type == TOKEN_EQ) return left == right;
    if (op->type == TOKEN_LE) return left <= right;
    return 0;
}

void statement() {
    Token *token = next_token();

    if (token->type == TOKEN_INT || token->type == TOKEN_FLOAT) {
        Token *ident = next_token();
        if (ident->type != TOKEN_IDENT) {
            printf("Error: Expected identifier\n");
            exit(1);
        }
        add_variable(ident->lexeme, token->type == TOKEN_FLOAT);
        expect(TOKEN_SEMICOLON);
    }
    else if (token->type == TOKEN_READ) {
        expect(TOKEN_LPAREN);
        Token *ident = next_token();
        if (ident->type != TOKEN_IDENT) {
            printf("Error: Expected identifier\n");
            exit(1);
        }
        int var_idx = find_variable(ident->lexeme);
        if (var_idx == -1) {
            printf("Error: Undefined variable %s\n", ident->lexeme);
            exit(1);
        }
        if (variables[var_idx].is_float) {
            scanf("%f", &variables[var_idx].value.f_val);
        } else {
            scanf("%d", &variables[var_idx].value.i_val);
        }
        expect(TOKEN_RPAREN);
        expect(TOKEN_SEMICOLON);
    }
    else if (token->type == TOKEN_WRITE) {
        expect(TOKEN_LPAREN);
        Token *ident = next_token();
        if (ident->type != TOKEN_IDENT) {
            printf("Error: Expected identifier\n");
            exit(1);
        }
        int var_idx = find_variable(ident->lexeme);
        if (var_idx == -1) {
            printf("Error: Undefined variable %s\n", ident->lexeme);
            exit(1);
        }
        if (variables[var_idx].is_float) {
            printf("%f\n", variables[var_idx].value.f_val);
        } else {
            printf("%d\n", variables[var_idx].value.i_val);
        }
        expect(TOKEN_RPAREN);
        expect(TOKEN_SEMICOLON);
    }
    else if (token->type == TOKEN_IDENT) {
        int var_idx = find_variable(token->lexeme);
        if (var_idx == -1) {
            printf("Error: Undefined variable %s\n", token->lexeme);
            exit(1);
        }
        expect(TOKEN_ASSIGN);
        float value = expression();
        if (variables[var_idx].is_float) {
            variables[var_idx].value.f_val = value;
        } else {
            variables[var_idx].value.i_val = (int)value;
        }
        expect(TOKEN_SEMICOLON);
    }
    else if (token->type == TOKEN_IF) {
        expect(TOKEN_LPAREN);
        int cond = condition();
        expect(TOKEN_RPAREN);
        expect(TOKEN_LBRACE);
        if (cond) {
            while (tokens[current_token].type != TOKEN_RBRACE) {
                statement();
            }
        } else {
            while (tokens[current_token].type != TOKEN_RBRACE) {
                current_token++;
            }
        }
        expect(TOKEN_RBRACE);
        if (tokens[current_token].type == TOKEN_ELSE) {
            next_token();
            expect(TOKEN_LBRACE);
            if (!cond) {
                while (tokens[current_token].type != TOKEN_RBRACE) {
                    statement();
                }
            } else {
                while (tokens[current_token].type != TOKEN_RBRACE) {
                    current_token++;
                }
            }
            expect(TOKEN_RBRACE);
        }
    }
    else if (token->type == TOKEN_WHILE) {
        int loop_start = current_token;
        expect(TOKEN_LPAREN);
        int cond_start = current_token;
        int cond = condition();
        expect(TOKEN_RPAREN);
        expect(TOKEN_LBRACE);
        int body_start = current_token;
        while (cond) {
            current_token = body_start;
            while (tokens[current_token].type != TOKEN_RBRACE) {
                statement();
            }
            current_token = cond_start;
            cond = condition();
        }
        while (tokens[current_token].type != TOKEN_RBRACE) {
            current_token++;
        }
        expect(TOKEN_RBRACE);
    }
}

void program() {
    expect(TOKEN_START);
    while (tokens[current_token].type != TOKEN_STOP) {
        statement();
    }
    expect(TOKEN_STOP);
}

int main() {
    char input[MAX_INPUT];
    char buffer[100];
    int input_pos = 0;
    printf("Enter your program (execution starts after 'stop;'):\n");

    while (input_pos < MAX_INPUT - 1) {
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            input[input_pos] = '\0';
            break;
        }

        int buffer_len = strlen(buffer);
        if (input_pos + buffer_len >= MAX_INPUT) {
            printf("Error: Input too long\n");
            exit(1);
        }
        strcpy(input + input_pos, buffer);
        input_pos += buffer_len;

        char *stop_pos = strstr(input, "stop;");
        if (stop_pos) {
            char *after_stop = stop_pos + 5;
            while (*after_stop && (isspace(*after_stop) || *after_stop == '\n')) after_stop++;
            if (*after_stop == '\0' || *after_stop == '\n') {
                input[input_pos] = '\0';
                break;
            }
        }
    }

    if (input_pos == 0) {
        printf("Error: No input provided\n");
        return 1;
    }

    tokenize(input);
    program();

    return 0;
}
