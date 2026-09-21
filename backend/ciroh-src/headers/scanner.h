#ifndef iroh_scanner_h
#define iroh_scanner_h

typedef enum {
  // Single-character tokens.
  TOKEN_LEFT_PAREN,
  TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE,
  TOKEN_RIGHT_BRACE,
  TOKEN_COMMA,
  TOKEN_DOT,
  TOKEN_MINUS,
  TOKEN_PLUS,
  TOKEN_COLON,
  TOKEN_CONTINUE,
  TOKEN_BREAK,

  TOKEN_SEMICOLON,
  TOKEN_SLASH,
  TOKEN_STAR,
  TOKEN_QUESTION,
  TOKEN_LEFT_BRAK,
  TOKEN_RIGHT_BRAK,
  // One or two character tokens.
  TOKEN_BANG,
  TOKEN_BANG_EQUAL,
  TOKEN_EQUAL,
  TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER,
  TOKEN_GREATER_EQUAL,
  TOKEN_LESS,
  TOKEN_LESS_EQUAL,
  // Literals.
  TOKEN_IDENTIFIER,
  TOKEN_STRING,
  TOKEN_NUMBER,
  // Keywords.
  TOKEN_AND,
  TOKEN_LEAF,
  TOKEN_ELSE,
  TOKEN_FALSE,
  TOKEN_FOR,
  TOKEN_CRAFT,
  TOKEN_IF,
  TOKEN_EMPTY,
  TOKEN_OR,
  TOKEN_SERVE,
  TOKEN_OFFER,
  TOKEN_ELDER,
  TOKEN_STEM,
  TOKEN_TRUE,
  TOKEN_BREW,
  TOKEN_WHILE,
  TOKEN_SWITCH,
  TOKEN_CASE,
  TOKEN_DEFAULT,

  TOKEN_ERROR,
  TOKEN_EOF
} TokenType;

typedef struct {
  TokenType type;
  const char *start;
  int length;
  int line;
} Token;

void initScanner(const char *source);

Token scanToken();

#endif
