#include "../headers/compiler.h"
#include "../headers/common.h"
#include "../headers/memory.h"
#include "../headers/object.h"
#include "../headers/scanner.h"

bool replMode;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG_PRINT_CODE
#include "../headers/debug.h"
#endif

/* ── Type Definitions ───────────────────────────────────────────────────────
 */

/* ParseFn must be declared before ParseRule since ParseRule holds two of them.
   Example: a prefix ParseFn for '-' would call unary(); an infix one calls
   binary(). */
typedef void (*ParseFn)(bool canAssign);

/* Holds the previous and current token so parse functions can inspect what was
   just scanned without re-scanning.
   Example: after advance(), parser.previous == the token we just consumed. */
typedef struct {
  Token previous;
  Token current;
  bool hadError;
  bool panicMode;
} Parser;

typedef struct {
  uint8_t index;
  bool isLocal;
} Upvalue;

typedef struct {
  Token name;
  int depth;
  bool isCaptured;

} Local;

typedef enum {
  TYPE_FUNCTION,
  TYPE_ANONYMOUS,
  TYPE_GETTER,
  TYPE_SCRIPT,
  TYPE_INITIALIZER,
  TYPE_METHOD,
} FunctionType;

typedef struct Compiler {
  int scopeDepth; // number of blocks surrounding?
  int localCount; // how many locals in a scope
  ObjFunction *function;
  FunctionType type;
  Local locals[UINT8_COUNT];
  int innermostLoopStart;
  int innermostLoopScopeDepth; // tracking the scope of the loops
  int breakJumps[256];         // ← collect break jumps
  int breakJumpCount;
  struct Compiler *enclosing;
  Upvalue upvalues[UINT8_COUNT];

} Compiler;

typedef struct ClassCompiler {
  struct ClassCompiler *enclosing;
  bool hasSuperclass;

} ClassCompiler;

Compiler *current = NULL;
ClassCompiler *currentClass = NULL;

/* Operator precedence levels, lowest to highest.
   Example: PREC_TERM (+ -) < PREC_FACTOR (* /) ensures 2+3*4 groups as 2+(3*4).
 */
typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_TERNARY,    // ?:
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

/* Maps a token type to the parse function for when it appears as a prefix,
   as an infix operator, and to its infix precedence level.
   Example: TOKEN_PLUS -> {NULL, binary, PREC_TERM} — no prefix meaning, but
   acts as an infix addition operator at PREC_TERM. */
typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

/* ── Forward Declarations ───────────────────────────────────────────────────
 */
/* Needed because expression() <-> parsePrecedence() <-> parse fns are mutually
   recursive, and getRule() + rules[] reference each other. */
static void expression();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);
static void error(const char *message);
static void statement();
static void ifStatement();
static void switchStatement();
static void whileStatement();
static void forStatement();
static void continueStatement();
static void breakStatement();
static int emitJump(uint8_t instruction);
static void patchJump(int offset);
static void declaration();
static void listLiteral(bool canAssign);
static void indexExpr(bool canAssign);
static void ternary(bool canAssign);
static void anonFunction(bool canAssign);
static void function(FunctionType type);

/* ── Globals ────────────────────────────────────────────────────────────────
 */

Parser parser;
Chunk *compilingChunk;

/* ── Chunk / Emit Helpers ───────────────────────────────────────────────────
 */

static void initCompiler(Compiler *compiler, FunctionType type) {
  compiler->enclosing = current; // ← save BEFORE updating current
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  compiler->function = NULL;
  compiler->type = type;
  compiler->innermostLoopScopeDepth = 0;
  compiler->innermostLoopStart = -1;
  compiler->breakJumpCount = 0;
  compiler->function = newFunction();
  current = compiler; // ← THEN update current

  if (type != TYPE_SCRIPT && type != TYPE_ANONYMOUS) {
    current->function->name =
        copyString(parser.previous.start,
                   parser.previous.length); // ← now current is correct
  }

  Local *local = &current->locals[current->localCount++];
  local->depth = 0;

  local->isCaptured = false;
  if (type != TYPE_FUNCTION) {
    local->name.start = "stem";
    local->name.length = 4;
  } else {
    local->name.start = "";
    local->name.length = 0;
  }
}
/* Returns the chunk currently being written to.
   Example: emitByte calls currentChunk() so all emit helpers share one target.
 */
static Chunk *currentChunk() { return &current->function->chunk; }

/* Appends one raw byte (opcode or operand) to the current chunk, tagging it
   with the line number of the token that produced it for runtime error
   reporting. Example: emitByte(OP_RETURN) adds a single return instruction. */
static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

/* Convenience wrapper to emit two bytes back-to-back in one call.
   Example: emitBytes(OP_CONSTANT, index) writes the opcode then its operand. */
static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

/* Emits the implicit return at the end of every compiled unit.
   Example: after compiling "1 + 2", endCompiler() calls this so the VM knows
   to stop executing. */
static void emitReturn() {
  if (current->type == TYPE_INITIALIZER) {
    emitBytes(OP_GET_LOCAL, 0);
  } else {
    emitByte(OP_NIL);
  }
  emitByte(OP_RETURN);
}

/* Emits OP_CONSTANT followed by the constant's pool index.
   Example: compiling the literal 42 calls emitConstant(42). */
static void emitConstant(Value value) {
  writeConstant(currentChunk(), value, parser.previous.line);
}

static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);

  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX)
    error("Loop body too large.");

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}

/* ── Error Helpers ──────────────────────────────────────────────────────────
 */

/* Core error reporter. Prints location and message to stderr, then sets
   panicMode to suppress cascading errors until the parser can synchronize.
   Example: errorAt(&parser.current, "Expect ')'") prints the offending token.
 */
static void errorAt(Token *token, const char *message) {
  if (parser.panicMode)
    return;
  parser.panicMode = true;

  fprintf(stderr, "[line %d] Error", token->line);
  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // scanner already stored the message in the token
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }
  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}

/* Reports an error at the token we just consumed (parser.previous).
   Example: after consuming a bad token, call error("Unexpected token."). */
static void error(const char *message) { errorAt(&parser.previous, message); }

/* Reports an error at the token we are about to consume (parser.current).
   Example: consume() calls this when the next token doesn't match expectations.
 */
static void errorAtCurrent(const char *message) {
  errorAt(&parser.current, message);
}

/* ── Scanner Interface ──────────────────────────────────────────────────────
 */

/* Moves to the next non-error token, storing the old current as previous.
   Error tokens from the scanner are reported immediately and skipped.
   Example: advance() is called at the start of every parse rule to consume
   the token the rule matched on. */
static void advance() {
  parser.previous = parser.current;
  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR)
      break;
    errorAtCurrent(parser.current.start);
  }
}

static bool check(TokenType type) { return parser.current.type == type; }

static bool match(TokenType type) {
  if (!check(type))
    return false;
  advance();
  return true;
}

/* Asserts the next token matches `type` and consumes it, or reports an error.
   Example: consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.") closes
   a grouping. */
static void consume(TokenType type, const char *message) {
  if (parser.current.type == type) {
    advance();
    return;
  }
  errorAtCurrent(message);
}

/* ── Parse Functions ────────────────────────────────────────────────────────
 */

/* Parses a numeric literal from the source text and emits it as a constant.
   Example: source token "3.14" -> strtod -> emitConstant(3.14). */
static void number(bool canAssign) {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

/* Handles prefix unary operators (currently only '-').
   Recursively compiles the operand at PREC_UNARY so that '--x' works correctly,
   then emits the negate instruction.
   Example: "-5" -> compile 5 -> OP_NEGATE. */
static void unary(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  parsePrecedence(PREC_UNARY);
  switch (operatorType) {
  case TOKEN_MINUS:
    emitByte(OP_NEGATE);
    break;
  case TOKEN_BANG:
    emitByte(OP_NOT);
    break;

  default:
    return;
  }
}

/* Parses a parenthesized sub-expression and discards the parentheses.
   The grouping itself emits no bytecode; it just adjusts parse precedence.
   Example: "(1 + 2) * 3" — grouping ensures '+' is compiled before '*'. */
static void grouping(bool canAssign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void anonFunction(bool canAssign) {
  function(TYPE_ANONYMOUS);
}

static void listLiteral(bool canAssign) {
  int count = 0;
  if (!check(TOKEN_RIGHT_BRAK)) {
    do {
      expression();
      count++;
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_BRAK, "Expect ']' after list declaration.");
  emitBytes(OP_BUILD_LIST, count);
}

/* Handles infix binary operators (+, -, *, /).
   Parses the right operand at one precedence level higher than the operator
   so that operators of equal precedence associate left-to-right.
   Example: "1 + 2 + 3" compiles as "(1 + 2) + 3". */
static void binary(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  ParseRule *rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
  case TOKEN_PLUS:
    emitByte(OP_ADD);
    break;
  case TOKEN_MINUS:
    emitByte(OP_SUBTRACT);
    break;
  case TOKEN_STAR:
    emitByte(OP_MULTIPLY);
    break;
  case TOKEN_SLASH:
    emitByte(OP_DIVIDE);
    break;
  case TOKEN_BANG_EQUAL:
    emitBytes(OP_EQUAL, OP_NOT);
    break;
  case TOKEN_EQUAL_EQUAL:
    emitByte(OP_EQUAL);
    break;
  case TOKEN_GREATER:
    emitByte(OP_GREATER);
    break;
  case TOKEN_GREATER_EQUAL:
    emitBytes(OP_LESS, OP_NOT);
    break;
  case TOKEN_LESS:
    emitByte(OP_LESS);
    break;
  case TOKEN_LESS_EQUAL:
    emitBytes(OP_GREATER, OP_NOT);
    break;
  default:
    return;
  }
}

static void literal(bool canAssign) {
  switch (parser.previous.type) {
  case TOKEN_FALSE:
    emitByte(OP_FALSE);
    break;
  case TOKEN_EMPTY:
    emitByte(OP_NIL);
    break;
  case TOKEN_TRUE:
    emitByte(OP_TRUE);
    break;
  default:
    return; // Unreachable.
  }
}

static void string(bool canAssign) {
  emitConstant(OBJ_VAL(
      copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static int makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    return constant;
  }
  return constant;
}

static uint8_t identifierConstant(Token *name) {
  ObjString *string = copyString(name->start, name->length);
  for (int i = 0; i < currentChunk()->constants.count; i++) {
    Value val = currentChunk()->constants.values[i];
    if (IS_STRING(val) && AS_STRING(val) == string) {
      return i;
    }
  }
  return makeConstant(OBJ_VAL(string));
}
static bool identifiersEqual(Token *a, Token *b) {
  if (a->length != b->length)
    return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler *compiler, Token *name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local *local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name)) {
      if (local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }

  return -1;
}

static int addUpvalue(Compiler *compiler, uint8_t index, bool isLocal) {
  int upvalueCount = compiler->function->upvalueCount;
  for (int i = 0; i < upvalueCount; i++) {
    Upvalue *upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->isLocal == isLocal) {
      return i;
    }
  }
  if (upvalueCount == UINT8_COUNT) {
    error("Too many closure variables in function.");
    return 0;
  }
  compiler->upvalues[upvalueCount].isLocal = isLocal;
  compiler->upvalues[upvalueCount].index = index;
  return compiler->function->upvalueCount++;
}

static int resolveUpvalue(Compiler *compiler, Token *name) {
  if (compiler->enclosing == NULL)
    return -1;

  int local = resolveLocal(compiler->enclosing, name);
  if (local != -1) {
    compiler->enclosing->locals[local].isCaptured = true;

    return addUpvalue(compiler, (uint8_t)local, true);
  }

  int upvalue = resolveUpvalue(compiler->enclosing, name);
  if (upvalue != -1) {
    return addUpvalue(compiler, (uint8_t)upvalue, false);
  }

  return -1;
}
static void namedVariable(Token name, bool canAssign) {
  uint8_t getOp, setOp;
  int arg = resolveLocal(current, &name);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else if ((arg = resolveUpvalue(current, &name)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(setOp, (uint8_t)arg);

  } else {
    emitBytes(getOp, (uint8_t)arg);
  }
}

static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}

static void and_(bool canAssign) {
  int endJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  parsePrecedence(PREC_AND);

  patchJump(endJump);
}
static void or_(bool canAssign) {
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  int endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(PREC_OR);
  patchJump(endJump);
}

static uint8_t argumentList() {
  uint8_t argCount = 0;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      expression();
      if (argCount == 255) {
        error("Can't have more than 255 arguments.");
      }
      argCount++;
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
  return argCount;
}
static void call(bool canAssign) {
  uint8_t argCount = argumentList();
  emitBytes(OP_CALL, argCount);
}

static void indexExpr(bool canAssign) {
  expression();
  consume(TOKEN_RIGHT_BRAK, "Expect ']' after index.");

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitByte(OP_INDEX_SET);
  } else {
    emitByte(OP_INDEX_GET);
  }
}

static void dot(bool canAssign) {
  advance(); // any token (including keywords) is valid as a property name
  uint8_t name = identifierConstant(&parser.previous);

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(OP_SET_PROPERTY, name);
  } else if (match(TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();
    emitBytes(OP_INVOKE, name);
    emitByte(argCount);
  } else {
    emitBytes(OP_GET_PROPERTY, name);
  }
}

/* ── Parse Rule Table ───────────────────────────────────────────────────────
 */

static void this_(bool canAssign) {

  if (currentClass == NULL) {
    error("Can't use 'stem' outside of a leaf.");
    return;
  }
  variable(false);
}
static Token syntheticToken(const char *text) {
  Token token;
  token.start = text;
  token.length = (int)strlen(text);
  return token;
}

static void super_(bool canAssign) {
  if (currentClass == NULL) {
    error("Can't use 'elder' outside of a leaf.");
  } else if (!currentClass->hasSuperclass) {
    error("Can't use 'elder' in a leaf with no parent.");
  }
  consume(TOKEN_DOT, "Expect '.' after 'elder'.");
  advance(); // any token is valid as a method name
  uint8_t name = identifierConstant(&parser.previous);

  namedVariable(syntheticToken("stem"), false);

  if (match(TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();
    namedVariable(syntheticToken("elder"), false);
    emitBytes(OP_SUPER_INVOKE, name);
    emitByte(argCount);
  } else {
    namedVariable(syntheticToken("elder"), false);
    emitBytes(OP_GET_SUPER, name);
  }
}

static void ternary(bool canAssign) {

  int thenJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  parsePrecedence(PREC_TERNARY);

  int elseJump = emitJump(OP_JUMP);
  patchJump(thenJump);
  emitByte(OP_POP);
  consume(TOKEN_COLON, "Expect ':' after then branch.");
  parsePrecedence(PREC_TERNARY);
  patchJump(elseJump);

  // x > 5 ? "big" : "small";
  //  current
}
/* Maps every token type to its prefix parse fn, infix parse fn, and infix
   precedence. Indexed by TokenType so getRule() is an O(1) array lookup.
   Example: rules[TOKEN_MINUS] = {unary, binary, PREC_TERM} — '-' can start a
   unary negation or appear as subtraction inside an expression. */
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRAK] = {listLiteral, indexExpr, PREC_CALL},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, dot, PREC_CALL},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_QUESTION] = {NULL, ternary, PREC_TERNARY},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, and_, PREC_AND},
    [TOKEN_LEAF] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_SWITCH] = {NULL, NULL, PREC_NONE},
    [TOKEN_CRAFT] = {anonFunction, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_EMPTY] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, or_, PREC_OR},
    [TOKEN_SERVE] = {NULL, NULL, PREC_NONE},
    [TOKEN_OFFER] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELDER] = {super_, NULL, PREC_NONE},
    [TOKEN_STEM] = {this_, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_BREW] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_CONTINUE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

/* ── Pratt Parser Core ──────────────────────────────────────────────────────
 */

/* Returns the ParseRule for a given token type via an O(1) table lookup.
   Example: getRule(TOKEN_STAR)->precedence == PREC_FACTOR. */
static ParseRule *getRule(TokenType type) { return &rules[type]; }

/* The heart of the Pratt parser. Consumes tokens and calls their parse
   functions as long as the next operator has higher precedence than
   `precedence`. Example: parsePrecedence(PREC_TERM) will compile "3 * 4"
   fully before returning, but stops before consuming a '+'. */
static void parsePrecedence(Precedence precedence) {
  advance();
  // advance moves to next token parser previous is the current
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }

  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}

/* Entry point for compiling any expression; starts at the lowest bindable
   precedence so the full expression is consumed.
   Example: expression() compiles "1 + 2 * 3" into the correct bytecode
   sequence. */
static void expression() { parsePrecedence(PREC_ASSIGNMENT); }

static void expressionStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emitByte(replMode ? OP_PRINT : OP_POP);
}

static void printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_PRINT);
}
static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void beginScope() { current->scopeDepth++; }
static void endScope() {
  current->scopeDepth--;
  while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth > current->scopeDepth) {
    if (current->locals[current->localCount - 1].isCaptured) {
      emitByte(OP_CLOSE_UPVALUE);
    } else {
      emitByte(OP_POP);
    }
    current->localCount--;
  }
}

static void patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}

static int emitJump(uint8_t instruction) {

  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk()->count - 2;
}
static void ifStatement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int thenJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();

  int elseJump = emitJump(OP_JUMP); // ← emit BEFORE patchJump(thenJump)

  patchJump(thenJump); // ← false path lands here
  emitByte(OP_POP);

  if (match(TOKEN_ELSE))
    statement();
  patchJump(elseJump); // ← true path lands here after then branch
}

static void switchStatement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'switch'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  consume(TOKEN_LEFT_BRACE, "Expect '{' after switch condition.");

  bool hadDefault = false;
  int endJumps[256];
  int endJumpCount = 0;

  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    if (hadDefault) {
      error("Default case must be last.");
      break;
    }

    if (match(TOKEN_CASE)) {
      emitByte(OP_DUP);
      expression();
      emitByte(OP_EQUAL);

      int caseJump = emitJump(OP_JUMP_IF_FALSE);
      emitByte(OP_POP);

      consume(TOKEN_COLON, "Expect ':' after case value.");

      while (!check(TOKEN_CASE) && !check(TOKEN_DEFAULT) &&
             !check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        statement();
      }

      endJumps[endJumpCount++] = emitJump(OP_JUMP);
      patchJump(caseJump);
      emitByte(OP_POP);

    } else if (match(TOKEN_DEFAULT)) {
      hadDefault = true;
      consume(TOKEN_COLON, "Expect ':' after default.");

      while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        statement();
      }

    } else {
      error("Expect 'case' or 'default'.");
      break;
    }
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after switch cases.");
  for (int i = 0; i < endJumpCount; i++) {
    patchJump(endJumps[i]);
  }

  emitByte(OP_POP);
}

static void continueStatement() {

  if (current->innermostLoopStart == -1) {
    error("Can't use 'continue' outside of a loop.");
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after continue.");
  for (int i = current->localCount - 1;
       i >= 0 && current->locals[i].depth > current->innermostLoopScopeDepth;
       i--) {
    emitByte(OP_POP);
  }
  emitLoop(current->innermostLoopStart);
}

static void breakStatement() {
  if (current->innermostLoopStart == -1) {
    error("Can't use 'break' outside of a loop.");
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after 'break'.");

  for (int i = current->localCount - 1;
       i >= 0 && current->locals[i].depth > current->innermostLoopScopeDepth;
       i--) {
    emitByte(OP_POP);
  }

  int breakJump = emitJump(OP_JUMP);
  current->breakJumps[current->breakJumpCount++] = breakJump;
}

static void returnStatement() {
  if (current->type == TYPE_SCRIPT) {
    error("Can't offer from top-level code.");
  }
  if (match(TOKEN_SEMICOLON)) {
    emitReturn();
  } else {
    if (current->type == TYPE_INITIALIZER) {
      error("Can't offer a value from an initializer.");
    }
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after offer value.");
    emitByte(OP_RETURN);
  }
}
static void statement() {
  if (match(TOKEN_SERVE)) {
    printStatement();
  } else if (match(TOKEN_OFFER)) {
    returnStatement();
  } else if (match(TOKEN_CONTINUE)) {
    continueStatement();
  } else if (match(TOKEN_BREAK)) {
    breakStatement();
  } else if (match(TOKEN_IF)) {
    ifStatement();
  } else if (match(TOKEN_WHILE)) {
    whileStatement();
  } else if (match(TOKEN_FOR)) {
    forStatement();

  } else if (match(TOKEN_SWITCH)) {
    switchStatement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
  } else {
    expressionStatement();
  }
}

static void markInitialized() {
  if (current->scopeDepth == 0)
    return;

  current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global) {
  if (current->scopeDepth > 0) {
    markInitialized();

    return;
  }

  emitBytes(OP_DEFINE_GLOBAL, global);
}

static void addLocal(Token name) {
  if (current->localCount == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }
  Local *local = &current->locals[current->localCount++];
  local->name = name;
  local->isCaptured = false;
  // local->depth=current->scoreDepth;
  local->depth = -1;
}

static void declareVariable() {
  if (current->scopeDepth == 0)
    return;

  Token *name = &parser.previous;
  for (int i = current->localCount - 1; i >= 0; i--) {
    Local *local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) {
      break;
    }

    if (identifiersEqual(name, &local->name)) {
      error("Already a variable with this name in this scope.");
    }
  }
  addLocal(*name);
}
static uint8_t parseVariable(const char *errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);
  declareVariable();
  if (current->scopeDepth > 0)
    return 0;
  return identifierConstant(&parser.previous);
}

/* ── Compiler Lifecycle ─────────────────────────────────────────────────────
 */

/* Finalizes a compilation unit by emitting an implicit return and, when the
   debug flag is set, disassembling the chunk for inspection.
   Example: called once at the end of compile() after the top-level
   expression.
 */
static ObjFunction *endCompiler() {
  emitReturn();
  ObjFunction *function = current->function;

#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassembleChunk(currentChunk(), function->name != NULL
                                         ? function->name->chars
                                         : "<script>");
  }
#endif
  current = current->enclosing;

  return function;
}

static void function(FunctionType type) {
  Compiler compiler;
  initCompiler(&compiler, type);
  beginScope();

  if (type != TYPE_GETTER) {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN)) {
      do {
        current->function->arity++;
        if (current->function->arity > 255) {
          errorAtCurrent("Can't have more than 255 parameters.");
        }
        uint8_t constant = parseVariable("Expect parameter name.");
        defineVariable(constant);
      } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
  }
  consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
  block();

  ObjFunction *function = endCompiler();
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));
  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
}

static void anonfunction(FunctionType type) {
  Compiler compiler;
  initCompiler(&compiler, type);
  beginScope();

  consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      current->function->arity++;
      if (current->function->arity > 255) {
        errorAtCurrent("Can't have more than 255 parameters.");
      }
      uint8_t constant = parseVariable("Expect parameter name.");
      defineVariable(constant);
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
  consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
  block();

  ObjFunction *function = endCompiler();
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));
  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
}

static void varDeclaration() {
  uint8_t global = parseVariable("Expect variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

  defineVariable(global);
}

static void whileStatement() {
  int loopStart = currentChunk()->count;
  int surroundingLoopStart = current->innermostLoopStart; // save outer
  current->innermostLoopStart = loopStart;

  int surroundingLoopScopeDepth = current->innermostLoopScopeDepth;
  current->innermostLoopScopeDepth = current->scopeDepth;

  int surroundingBreakJumpCount = current->breakJumpCount;

  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int exitJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();
  emitLoop(loopStart);

  patchJump(exitJump);
  emitByte(OP_POP);
  while (current->breakJumpCount > surroundingBreakJumpCount) {
    patchJump(current->breakJumps[--current->breakJumpCount]);
  }
  current->innermostLoopStart = surroundingLoopStart;
  current->innermostLoopScopeDepth = surroundingLoopScopeDepth;
}

static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON)
      return;
    switch (parser.current.type) {
    case TOKEN_LEAF:
    case TOKEN_CRAFT:
    case TOKEN_BREW:
    case TOKEN_FOR:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_SERVE:
    case TOKEN_OFFER:
      return;

    default:; // Do nothing.
    }

    advance();
  }
}

static void method() {
  advance(); // accept any token as method name

  if (check(TOKEN_LEFT_BRACE)) {
    // getter — store as "get$name"
    char prefixed[256];
    memcpy(prefixed, "get$", 4);
    memcpy(prefixed + 4, parser.previous.start, parser.previous.length);
    uint8_t constant = makeConstant(OBJ_VAL(copyString(prefixed, parser.previous.length + 4)));
    function(TYPE_GETTER);
    emitBytes(OP_METHOD, constant);
    return;
  }

  uint8_t constant = identifierConstant(&parser.previous);
  FunctionType type = TYPE_METHOD;
  if (parser.previous.length == 4 &&
      memcmp(parser.previous.start, "init", 4) == 0) {
    type = TYPE_INITIALIZER;
  }
  function(type);
  emitBytes(OP_METHOD, constant);
}

static void staticMethod() {
  advance(); // consume method name
  // store as "$name" in the methods table
  char prefixed[256];
  prefixed[0] = '$';
  memcpy(prefixed + 1, parser.previous.start, parser.previous.length);
  uint8_t constant = makeConstant(OBJ_VAL(copyString(prefixed, parser.previous.length + 1)));
  function(TYPE_FUNCTION); // no 'this'
  emitBytes(OP_METHOD, constant);
}

static void classDeclaration() {
  consume(TOKEN_IDENTIFIER, "Expect class name.");
  Token className = parser.previous;

  uint8_t nameConstant = identifierConstant(&parser.previous);
  declareVariable();

  emitBytes(OP_CLASS, nameConstant);
  defineVariable(nameConstant);
  ClassCompiler classCompiler;
  classCompiler.hasSuperclass = false;
  classCompiler.enclosing = currentClass;
  currentClass = &classCompiler;
  if (match(TOKEN_LESS)) {
    consume(TOKEN_IDENTIFIER, "Expect superclass name.");
    variable(false);
    if (identifiersEqual(&className, &parser.previous)) {
      error("A class can't inherit from itself.");
    }
    beginScope();
    addLocal(syntheticToken("elder"));
    defineVariable(0);
    namedVariable(className, false);
    emitByte(OP_INHERIT);
    classCompiler.hasSuperclass = true;
  }
  namedVariable(className, false);

  consume(TOKEN_LEFT_BRACE, "Expect '{' before leaf body.");

  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    if (match(TOKEN_LEAF)) {
      staticMethod();
    } else {
      method();
    }
  }
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after leaf body.");
  emitByte(OP_POP);
  if (classCompiler.hasSuperclass) {
    endScope();
  }
  currentClass = currentClass->enclosing;
}
static void forStatement() {

  beginScope();
  int surroundingLoopStart = current->innermostLoopStart;
  int surroundingLoopScopeDepth = current->innermostLoopScopeDepth;
  int surroundingBreakJumpCount = current->breakJumpCount;

  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
  if (match(TOKEN_SEMICOLON)) {
    // No initializer.
  } else if (match(TOKEN_BREW)) {
    varDeclaration();
  } else {
    expressionStatement();
  }

  int loopStart = currentChunk()->count;
  current->innermostLoopStart = currentChunk()->count;
  current->innermostLoopScopeDepth = current->scopeDepth;

  int exitJump = -1;
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

    // Jump out of the loop if the condition is false.
    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // Condition.
  }

  if (!match(TOKEN_RIGHT_PAREN)) {
    int bodyJump = emitJump(OP_JUMP);
    int incrementStart = currentChunk()->count;
    expression();
    emitByte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

    emitLoop(loopStart);
    loopStart = incrementStart;
    current->innermostLoopStart = loopStart;
    patchJump(bodyJump);
  }

  statement();
  emitLoop(loopStart);
  if (exitJump != -1) {
    patchJump(exitJump);
    emitByte(OP_POP); // Condition.
  }
  while (current->breakJumpCount > surroundingBreakJumpCount) {
    patchJump(current->breakJumps[--current->breakJumpCount]);
  }
  current->innermostLoopStart = surroundingLoopStart;
  current->innermostLoopScopeDepth = surroundingLoopScopeDepth;

  endScope();
}

static void funDeclaration() {
  uint8_t global = parseVariable("Expect function name.");
  markInitialized();
  function(TYPE_FUNCTION);
  defineVariable(global);
}
static void declaration() {
  if (match(TOKEN_LEAF)) {
    classDeclaration();
  } else if (match(TOKEN_CRAFT)) {
    funDeclaration();
  } else if (match(TOKEN_BREW)) {
    varDeclaration();
  } else {
    statement();
  }
  if (parser.panicMode)
    synchronize();
}

/* Compiles `source` into `chunk`. Returns true on success, false if any error
   was encountered. This is the sole public entry point into the compiler.
   Example: compile("1 + 2", &chunk) fills chunk with OP_CONSTANT, OP_ADD,
   OP_RETURN and returns true. */
ObjFunction *compile(const char *source) {
  initScanner(source);
  Compiler compiler;
  initCompiler(&compiler, TYPE_SCRIPT);
  parser.hadError = false;
  parser.panicMode = false;

  advance();
  while (!match(TOKEN_EOF)) {
    declaration();
  }
  ObjFunction *function = endCompiler();
  return parser.hadError ? NULL : function;
}

void markCompilerRoots() {
  Compiler *compiler = current;
  while (compiler != NULL) {
    markObject((Obj *)compiler->function);
    compiler = compiler->enclosing;
  }
}
