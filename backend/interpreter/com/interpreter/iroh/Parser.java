package com.interpreter.iroh;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import static com.interpreter.iroh.TokenType.*;
class Parser {
    private static class ParseError extends RuntimeException {}

    private final List<Token> tokens;
    private int loopDepth = 0;
    private int switchDepth = 0;
    private int current = 0;

    Parser(List<Token> tokens) {
        this.tokens = tokens;
    }

    List<Stmt> parse() {
        List<Stmt> statements = new ArrayList<>();
        while (!isAtEnd()) {
            statements.add(declaration());
        }
        return statements;
    }

    private Stmt declaration() {
        try {
			if (match(LEAF)) return classDeclaration();
            if (match(BREW)) return varDeclaration();
            if (match(CRAFT)) return funDeclaration("function",false);

            return statement();
        } catch (ParseError error) {
            synchronize();
            return null;
        }
    }

	private Stmt classDeclaration(){
		Token name=consume(IDENTIFIER,"Expected a class name");
		 Expr.Variable superclass = null;
    if (match(LESS)) {
      consume(IDENTIFIER, "Expect superclass name.");
      superclass = new Expr.Variable(previous());
    }
		consume(LEFT_BRACE, "Expect '{' before class body.");
		
		List<Stmt.Function> methods = new ArrayList<>();
		while (!check(RIGHT_BRACE) && !isAtEnd()) {
			if(match(LEAF)){
			methods.add(funDeclaration("static",true));
			}else{
			methods.add(funDeclaration("method",false));
			}
		}
		
		consume(RIGHT_BRACE, "Expect '}' after class body.");
	

		return new Stmt.Class(name,superclass,methods);

	}

    private List<Stmt> block() {
        List<Stmt> stmts = new ArrayList<>();
        while (!check(RIGHT_BRACE) && !isAtEnd()) {
            Stmt stmt = declaration();
            stmts.add(stmt);
        }
        consume(RIGHT_BRACE, "Expected } after opening braces");
        return stmts;
    }

    private Stmt varDeclaration() {
        Token name = consume(IDENTIFIER, "Expect variable name");
        Expr initializer = null;
        if (match(EQUAL)) {
            initializer = expression();
        }
        consume(SEMICOLON, "Expect ';' after variable declaration.");
        return new Stmt.Var(name, initializer);
    }

    private Stmt.Function funDeclaration(String kind, boolean isStatic) {
  Token name = consume(IDENTIFIER, "Expect method name.");

  if (match(LEFT_PARENT)) {
    List<Token> params = new ArrayList<>();

    if (!check(RIGHT_PARENT)) {
      do {
        params.add(consume(IDENTIFIER, "Expect parameter name."));
      } while (match(COMMA));
    }

    consume(RIGHT_PARENT, "Expect ')' after parameters.");
    consume(LEFT_BRACE, "Expect '{' before body.");

    List<Stmt> body = block();
    return new Stmt.Function(name, params, body, isStatic,false);
  }


  // getter form
  consume(LEFT_BRACE, "Expect '{' before getter body.");
  List<Stmt> body = block();


  return new Stmt.Function(name, new ArrayList<>(), body, isStatic,true);
	}

    private Stmt statement() {
        if (match(SERVE)) return printStatement();
        if (match(IF)) return IFStatement();
        if (match(FOR)) return ForStatement();
        if (match(OFFER)) return ReturnStatement();
        if (match(WHILE)) return WhileStatement();
        if (match(BREAK)) return breakStatement();
        if (match(CONTINUE)) return continueStatement();
        if (match(SWITCH)) return switchStatement();
        if (match(LEFT_BRACE)) return new Stmt.Block(block());
        return expressionStatement();
    }

    private Stmt breakStatement() {
        Token keyword = previous();
        if (loopDepth == 0 && switchDepth == 0) {
            error(keyword, "'break' must be inside a loop or switch.");
        }
        consume(SEMICOLON, "Expect ';' after break.");
        return new Stmt.Break(keyword);
    }

    private Stmt continueStatement() {
        Token keyword = previous();
        if (loopDepth == 0) {
            error(keyword, "'continue' must be inside a loop.");
        }
        consume(SEMICOLON, "Expect ';' after continue.");
        return new Stmt.Continue(keyword);
    }

    private Stmt switchStatement() {
        consume(LEFT_PARENT, "Expect '(' after 'switch'.");
        Expr subject = expression();
        consume(RIGHT_PARENT, "Expect ')' after switch value.");
        consume(LEFT_BRACE, "Expect '{' after switch condition.");

        switchDepth++;
        List<Stmt.SwitchCase> cases = new ArrayList<>();
        boolean hadDefault = false;

        while (!check(RIGHT_BRACE) && !isAtEnd()) {
            if (hadDefault) {
                error(peek(), "Default case must be last.");
                break;
            }
            if (match(CASE)) {
                Expr value = expression();
                consume(COLON, "Expect ':' after case value.");
                List<Stmt> body = new ArrayList<>();
                while (!check(CASE) && !check(DEFAULT) && !check(RIGHT_BRACE) && !isAtEnd()) {
                    body.add(declaration());
                }
                cases.add(new Stmt.SwitchCase(value, body));
            } else if (match(DEFAULT)) {
                hadDefault = true;
                consume(COLON, "Expect ':' after default.");
                List<Stmt> body = new ArrayList<>();
                while (!check(RIGHT_BRACE) && !isAtEnd()) {
                    body.add(declaration());
                }
                cases.add(new Stmt.SwitchCase(null, body));
            } else {
                error(peek(), "Expect 'case' or 'default'.");
                break;
            }
        }

        switchDepth--;
        consume(RIGHT_BRACE, "Expect '}' after switch cases.");
        return new Stmt.Switch(subject, cases);
    }

    private Stmt ReturnStatement() {
        Token keyword = previous();
        Expr value = null;
        if (!check(SEMICOLON)) {
            value = expression();
        }
        consume(SEMICOLON, "Expect ';' after offer value.");
        return new Stmt.Return(keyword, value);
    }

    private Stmt ForStatement() {
        consume(LEFT_PARENT, "Expect '(' after 'for'");
        Stmt initializer;
        if (match(SEMICOLON)) {
            initializer = null;
        } else if (match(BREW)) {
            initializer = varDeclaration();
        } else {
            initializer = expressionStatement();
        }

        Expr condition = null;
        if (!check(SEMICOLON)) {
            condition = expression();
        }
        consume(SEMICOLON, "Expect ';' after loop condition.");

        Expr increment = null;
        if (!check(RIGHT_PARENT)) {
            increment = expression();
        }
        consume(RIGHT_PARENT, "Expect ')' after for clauses.");

        loopDepth++;
        Stmt body = statement();
        loopDepth--;

        if (increment != null) {
            body = new Stmt.Block(
                Arrays.asList(body, new Stmt.Expression(increment)));
        }

        if (condition == null) condition = new Expr.Literal(true);
        body = new Stmt.While(condition, body);

        if (initializer != null) {
            body = new Stmt.Block(Arrays.asList(initializer, body));
        }
        return body;
    }

    private Stmt WhileStatement() {
        consume(LEFT_PARENT, "Expect '(' after 'while'");
        Expr condition = expression();
        consume(RIGHT_PARENT, "Expect ')' after 'while'");
        loopDepth++;
        Stmt thenBranch = statement();
        loopDepth--;
        return new Stmt.While(condition, thenBranch);
    }

    private Stmt IFStatement() {
        consume(LEFT_PARENT, "Expect '(' after 'if'");
        Expr condition = expression();
        consume(RIGHT_PARENT, "Expect ')' after 'if'");
        Stmt thenBranch = statement();
        Stmt elseBranch = null;
        if (match(ELSE)) {
            elseBranch = statement();
        }
        return new Stmt.IF(condition, thenBranch, elseBranch);
    }

    private Stmt printStatement() {
        Expr value = expression();
        consume(SEMICOLON, "Expected ; after a print statement");
        return new Stmt.Print(value);
    }

    private Stmt expressionStatement() {
        Expr expr = expression();
        consume(SEMICOLON, "Expect ';' after expression.");
        return new Stmt.Expression(expr);
    }

    private Expr expression() {
        return assignment();
    }

    private Expr assignment() {
        Expr expr = comma();
        if (match(EQUAL)) {
            Token equal = previous();
            Expr assign = assignment();
            if (expr instanceof Expr.Variable) {
                Token name = ((Expr.Variable) expr).name;
                return new Expr.Assign(name, assign);
            }
			else if (expr instanceof Expr.Get) {
				Expr.Get get = (Expr.Get)expr;
				return new Expr.Set(get.object, get.name, assign);
        }}
        return expr;
    }

    private Expr comma() {
        Expr expr = logic_or();
        while (match(COMMA)) {
            Token operator = previous();
            Expr right = logic_or();
            expr = new Expr.Binary(expr, operator, right);
        }
        return expr;
    }

    private Expr logic_or() {
        Expr expr = logic_and();
        while (match(OR)) {
            Token or = previous();
            Expr right = logic_and();
            expr = new Expr.Logical(expr, or, right);
        }
        return expr;
    }

    private Expr logic_and() {
        Expr expr = ternary();
        while (match(AND)) {
            Token and = previous();
            Expr right = ternary();
            expr = new Expr.Logical(expr, and, right);
        }
        return expr;
    }

    private Expr ternary() {
        Expr expr = equality();
        if (match(QUESTION)) {
            Token operator = previous();
            Expr true_branch = expression();
            consume(COLON, ": expected for Ternary expression");
            Expr false_branch = ternary();
            return new Expr.Ternary(expr, operator, true_branch, false_branch);
        }
        return expr;
    }

    private Expr equality() {
        Expr expr = comparison();
        while (match(BANG_EQUAL, EQUAL_EQUAL)) {
            Token operator = previous();
            Expr right = comparison();
            expr = new Expr.Binary(expr, operator, right);
        }
        return expr;
    }

    private Expr comparison() {
        Expr expr = term();
        while (match(LESS, LESS_EQUAL, GREATER, GREATER_EQUAL)) {
            Token operator = previous();
            Expr right = term();
            expr = new Expr.Binary(expr, operator, right);
        }
        return expr;
    }

    private Expr term() {
        Expr expr = factor();
        while (match(MINUS, PLUS)) {
            Token operator = previous();
            Expr right = factor();
            expr = new Expr.Binary(expr, operator, right);
        }
        return expr;
    }

    private Expr factor() {
        Expr expr = unary();
        while (match(SLASH, STAR)) {
            Token operator = previous();
            Expr right = unary();
            expr = new Expr.Binary(expr, operator, right);
        }
        return expr;
    }

    private Expr unary() {
        if (match(BANG, MINUS)) {
            Token operator = previous();
            Expr right = unary();
            return new Expr.Unary(operator, right);
        }
        return call();
    }

    private Expr call() {
        Expr expr = primary();
        while (true) {
            if (match(LEFT_PARENT)) {
                expr = finishCall(expr);}
			else if (match(LEFT_BRAK)) {
                expr = finishIndex(expr);}
			else if (match(DOT)) {
				if (isAtEnd()) throw error(peek(), "Expect property name after '.'.");
				Token name = advance();
				expr = new Expr.Get(expr, name);
            } else {
                break;
            }
        }
        return expr;
    }

private Expr finishIndex(Expr object) {
  Token bracket = previous(); 

  Expr index = expression();
  consume(RIGHT_BRAK, "Expect ']' after index.");

  return new Expr.Index(object, bracket, index);
}

    private Expr finishCall(Expr expr) {
        List<Expr> arguments = new ArrayList<>();
        if (!check(RIGHT_PARENT)) {
            do {
                if (arguments.size() >= 255) {
                    error(peek(), "Can't have more than 255 arguments.");
                }
                arguments.add(logic_or());
            } while (match(COMMA));
        }
        Token paren = consume(RIGHT_PARENT, "Expect ')' after arguments.");
        return new Expr.Call(expr, paren, arguments);
    }

	private Expr listLiteral() {
		List<Expr> elements = new ArrayList<>();

		if (!check(RIGHT_BRAK)) {
			do {
				elements.add(logic_or());
			} while (match(COMMA));
		}

		consume(RIGHT_BRAK, "Expect ']' after list.");

		return new Expr.ListExpr(elements);
	}

    private Expr primary() {

		if (match(LEFT_BRAK)){
			return listLiteral();
		}




        if (match(FALSE)) return new Expr.Literal(false);
        if (match(TRUE)) return new Expr.Literal(true);
        if (match(EMPTY)) return new Expr.Literal(null);

		if(match(STEM)) return new Expr.This(previous());
		  if (match(ELDER)) {
      Token keyword = previous();
      consume(DOT, "Expect '.' after 'elder'.");
      if (isAtEnd()) throw error(peek(), "Expect parent method name.");
      Token method = advance();
      return new Expr.Super(keyword, method);
    }

        if (match(NUMBER, STRING)) {
            return new Expr.Literal(previous().literal);
        }

        if (match(IDENTIFIER)) {
            return new Expr.Variable(previous());
        }

        if (match(LEFT_PARENT)) {
            Expr expr = expression();
            consume(RIGHT_PARENT, "Expect ')' after expression.");
            return new Expr.Grouping(expr);
        }

        if (match(SLASH, STAR)) {
            Token operator = previous();
            error(operator, "Expected a valid left operand.");
            factor();
            return new Expr.Literal(null);
        }

        if (match(LESS, LESS_EQUAL, GREATER, GREATER_EQUAL)) {
            Token operator = previous();
            error(operator, "Expected a valid left operand.");
            comparison();
            return new Expr.Literal(null);
        }

        if (match(BANG_EQUAL, EQUAL_EQUAL)) {
            Token operator = previous();
            error(operator, "Expected a valid left operand.");
            equality();
            return new Expr.Literal(null);
        }

        if (match(CRAFT)) {
            consume(LEFT_PARENT, "Expect '(' after 'craft'.");
            List<Token> parameters = new ArrayList<>();
            if (!check(RIGHT_PARENT)) {
                do {
                    if (parameters.size() >= 255) {
                        error(peek(), "Can't have more than 255 parameters.");
                    }
                    parameters.add(consume(IDENTIFIER, "Expect parameter name."));
                } while (match(COMMA));
            }
            consume(RIGHT_PARENT, "Expect ')' after parameters.");
            consume(LEFT_BRACE, "Expect '{' before function body.");
            List<Stmt> body = block();
            return new Expr.AnonFunctionExpr(parameters, body);
        }

        throw error(peek(), "Expect expression.");
    }

    private Token consume(TokenType type, String message) {
        if (check(type)) return advance();
        throw error(peek(), message);
    }

    private boolean match(TokenType... types) {
        for (TokenType type : types) {
            if (check(type)) {
                advance();
                return true;
            }
        }
        return false;
    }

    private boolean check(TokenType type) {
        if (isAtEnd()) return false;
        return peek().type == type;
    }

    private Token advance() {
        if (!isAtEnd()) current++;
        return previous();
    }

    private boolean isAtEnd() {
        return peek().type == EOF;
    }

    private Token peek() {
        return tokens.get(current);
    }

    private Token previous() {
        return tokens.get(current - 1);
    }

    private ParseError error(Token token, String message) {
        Iroh.error(token, message);
        return new ParseError();
    }

    private void synchronize() {
        advance();
        while (!isAtEnd()) {
            if (previous().type == SEMICOLON) return;
            switch (peek().type) {
                case LEAF:
                case CRAFT:
                case BREW:
                case FOR:
                case IF:
                case WHILE:
                case SERVE:
                case OFFER:
                case CONTINUE:
                    return;
            }
            advance();
        }
    }
}
