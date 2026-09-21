#include <stdio.h>
#include <string.h>

#include "../headers/common.h"
#include "../headers/scanner.h"

typedef struct {
  const char *start;
  const char *current;
  int line;
} Scanner;

Scanner scanner;

void initScanner(const char *source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}
static bool isAtEnd() { return *scanner.current == '\0'; }

static Token makeToken(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;
  return token;
}

static Token errorToken(const char *message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;
  return token;
}

static char advance() {
  scanner.current++;
  return scanner.current[-1];
}
static bool match(char expected) {
  if (isAtEnd())
    return false;
  if (*scanner.current != expected)
    return false;
  scanner.current++;
  return true;
}
static char peek() { return *scanner.current; }
static char peekNext() {
  if (isAtEnd())
    return '\0';
  return scanner.current[1];
}
static void skipWhitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      advance();
      break;
    case '\n':
      scanner.line++;
      advance();
      break;
    case '/':
      if (peekNext() == '/') {
        while (peek() != '\n' && !isAtEnd()) {
          advance();
        }
      } else {
        return;
      }
      break;
    default:
      return;
    }
  }
}

static Token string() {
  // "hello"
  //  ^
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n') {
      scanner.line++;
    }
    advance();
  }
  if (isAtEnd())
    return errorToken("Unterminated string.");
  advance();

  return makeToken(TOKEN_STRING);
}

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool isDigit(char c) { return c >= '0' && c <= '9'; }

static Token number() {
  while (isDigit(peek()))
    advance();

  if (peek() == '.' && isDigit(peekNext())) {
    advance();
    while (isDigit(peek()))
      advance();
  }

  return makeToken(TOKEN_NUMBER);
}

static TokenType checkKeyword(int start, int length, const char *rest,
                              TokenType type) {
  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, rest, length) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {

  switch (scanner.start[0]) {
  case 'a':
    return checkKeyword(1, 2, "nd", TOKEN_AND);
  case 'b':
    if (scanner.current - scanner.start > 1 && scanner.start[1] == 'r') {
      // brew (4 chars) or break (5 chars)
      TokenType t = checkKeyword(2, 2, "ew", TOKEN_BREW);
      if (t != TOKEN_IDENTIFIER)
        return t;
      return checkKeyword(2, 3, "eak", TOKEN_BREAK);
    }
    break;
  case 'c':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'a':
        return checkKeyword(2, 2, "se", TOKEN_CASE);
      case 'r':
        return checkKeyword(2, 3, "aft", TOKEN_CRAFT);
      case 'o':
        return checkKeyword(2, 6, "ntinue", TOKEN_CONTINUE);
      }
    }
    break;
  case 'd':
    return checkKeyword(1, 6, "efault", TOKEN_DEFAULT);
  case 'e':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'l':
        if (scanner.current - scanner.start > 2) {
          switch (scanner.start[2]) {
          case 's':
            return checkKeyword(3, 1, "e", TOKEN_ELSE); // else (4)
          case 'd':
            return checkKeyword(3, 2, "er", TOKEN_ELDER); // elder (5)
          }
        }
        break;
      case 'm':
        return checkKeyword(2, 3, "pty", TOKEN_EMPTY); // empty (5)
      }
    }
    break;
  case 'f':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'a':
        return checkKeyword(2, 3, "lse", TOKEN_FALSE);
      case 'o':
        return checkKeyword(2, 1, "r", TOKEN_FOR);
      }
    }
    break;
  case 'i':
    return checkKeyword(1, 1, "f", TOKEN_IF);
  case 'l':
    return checkKeyword(1, 3, "eaf", TOKEN_LEAF);
  case 'n':
    break; // no keywords start with 'n'
  case 'o':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'r':
        return checkKeyword(1, 1, "r", TOKEN_OR); // or (2)
      case 'f':
        return checkKeyword(1, 4, "ffer", TOKEN_OFFER); // offer (5)
      }
    }
    break;
  case 's':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'e':
        return checkKeyword(2, 3, "rve", TOKEN_SERVE); // serve (5)
      case 't':
        return checkKeyword(2, 2, "em", TOKEN_STEM); // stem (4)
      case 'w':
        return checkKeyword(2, 4, "itch", TOKEN_SWITCH); // switch (6)
      }
    }
    break;
  case 't':
    return checkKeyword(1, 3, "rue", TOKEN_TRUE);
  case 'w':
    return checkKeyword(1, 4, "hile", TOKEN_WHILE);
  }
  return TOKEN_IDENTIFIER;
}

static Token identifier() {
  while (isAlpha(peek()) || isDigit(peek()))
    advance();
  return makeToken(identifierType());
}

Token scanToken() {
  skipWhitespace();

  scanner.start = scanner.current;
  if (isAtEnd()) {
    return makeToken(TOKEN_EOF);
  }
  char c = advance();

  if (isAlpha(c))
    return identifier();
  if (isDigit(c))
    return number();

  switch (c) {
  case '(':
    return makeToken(TOKEN_LEFT_PAREN);
  case ')':
    return makeToken(TOKEN_RIGHT_PAREN);
  case '{':
    return makeToken(TOKEN_LEFT_BRACE);
  case '}':
    return makeToken(TOKEN_RIGHT_BRACE);
  case '[':
    return makeToken(TOKEN_LEFT_BRAK);
  case ']':
    return makeToken(TOKEN_RIGHT_BRAK);
  case ';':
    return makeToken(TOKEN_SEMICOLON);
  case ',':
    return makeToken(TOKEN_COMMA);
  case '.':
    return makeToken(TOKEN_DOT);
  case '-':
    return makeToken(TOKEN_MINUS);
  case '+':
    return makeToken(TOKEN_PLUS);
  case '/':
    return makeToken(TOKEN_SLASH);
  case '*':
    return makeToken(TOKEN_STAR);
  case ':':
    return makeToken(TOKEN_COLON);
  case '?':
    return makeToken(TOKEN_QUESTION);
  case '!':
    return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
  case '=':
    return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
  case '<':
    return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
  case '>':
    return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
  case '"':
    return string();
  }
  return errorToken("Unexpected character.");
}
