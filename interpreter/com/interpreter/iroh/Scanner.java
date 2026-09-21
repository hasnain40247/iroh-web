package com.interpreter.iroh;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static com.interpreter.iroh.TokenType.*;

class Scanner {
	private final String source;
	private final List<Token> tokens = new ArrayList<>(); // the tokens > are a list
		
	private int start=0;
	private int current=0;
	private int line=1;

	private static final Map<String, TokenType> keywords;

  static {
    keywords = new HashMap<>();
    keywords.put("break",  BREAK);
    keywords.put("continue", CONTINUE);
    keywords.put("switch",   SWITCH);
    keywords.put("case",     CASE);
    keywords.put("default",  DEFAULT);
    keywords.put("and",    AND);
    keywords.put("leaf",   LEAF);
    keywords.put("else",   ELSE);
    keywords.put("false",  FALSE);
    keywords.put("for",    FOR);
    keywords.put("craft",  CRAFT);
    keywords.put("if",     IF);
    keywords.put("empty",  EMPTY);
    keywords.put("or",     OR);
    keywords.put("serve",  SERVE);
    keywords.put("offer",  OFFER);
    keywords.put("elder",  ELDER);
    keywords.put("stem",   STEM);
    keywords.put("true",   TRUE);
    keywords.put("brew",   BREW);
    keywords.put("while",  WHILE);
  }

	Scanner(String source){
		this.source=source;
	}

	List<Token> scanTokens(){
		while(!isAtEnd()){
			start=current;
			scanToken();
		}

		tokens.add(new Token(EOF,"",null,line));
		return tokens;
	}

	private boolean isAtEnd(){
		return current>=source.length();
	}
	private void scanToken(){
		char c=advance();
		switch(c) {
	  case '(': addToken(LEFT_PARENT); break;
      case ')': addToken(RIGHT_PARENT); break;
	 case '[': addToken(LEFT_BRAK); break;
      case ']': addToken(RIGHT_BRAK); break;
      case '{': addToken(LEFT_BRACE); break;
      case '}': addToken(RIGHT_BRACE); break;
      case ',': addToken(COMMA); break;
      case '.': addToken(DOT); break;
      case '-': addToken(MINUS); break;
      case '+': addToken(PLUS); break;
      case ';': addToken(SEMICOLON); break;
      case '*': addToken(STAR); break;
	  case '!':
        addToken(match('=') ? BANG_EQUAL : BANG);
        break;
      case '=':
        addToken(match('=') ? EQUAL_EQUAL : EQUAL);
        break;
      case '<':
        addToken(match('=') ? LESS_EQUAL : LESS);
        break;
      case '>':
        addToken(match('=') ? GREATER_EQUAL : GREATER);
        break;
	  case '?':
		addToken(QUESTION); break;
	  case ':': addToken(COLON); break;
	case '/':
		if (match('/')){
			while(peek() != '\n' && !isAtEnd()) advance();
		}else if (match('*')){
				comment();
			  }else{addToken(SLASH);} break;
      case ' ':
      case '\r':
      case '\t':
        // Ignore whitespace.
        break;

      case '\n':
        line++;
        break;
	  case '"': string(); break;
	  default:
				if(isDigit(c)){
					number();
				}else if(isAlpha(c)){
					identifier();
				}else{
				Iroh.error(line, "Unexpected character.");}
				break;
		}
	}

	private char advance(){
		return source.charAt(current++);
	}

	private void addToken(TokenType type){
		addToken(type,null);
	}
	private void addToken(TokenType type, Object literal){
		String text=source.substring(start,current);
		tokens.add(new Token(type,text,literal,line));
	}

	private boolean match(char expected){
		if(isAtEnd())return false;
		if(source.charAt(current)!= expected) return false;

		current++;
		return true;
	}
	private char peek(){
		if (isAtEnd()) return '\0';
		return source.charAt(current);
	}
	private void string(){
		while(peek()!='"' && !isAtEnd()){
			if (peek()=='\n') line++;
			advance(); // advance unbtil the end of the sting
		}

		if(isAtEnd()){
			Iroh.error(line,"Unterminated String");
		return;
		}
		advance();
		String value=source.substring(start+1,current-1); //trim the stirng to remove quote
		addToken(STRING,value);
	}

	private boolean isDigit(char c){
		return c >='0' && c <= '9';
	}

	private void number(){
		while(isDigit(peek())) advance();
		if(peek()=='.' && isDigit(peekNext())){
			advance();
		}
		while( isDigit(peek())) advance();

		addToken(NUMBER,Double.parseDouble(source.substring(start,current)));
	}

	private char peekNext(){
		if(current+1 >= source.length()) return '\0';
		return source.charAt(current+1);
	}
	private void identifier(){
	while(isAlphaNumeric(peek())) advance();

	String text=source.substring(start,current);
	TokenType type=keywords.get(text);
	if (type==null) type=IDENTIFIER;
	addToken(type);
	}

	 private boolean isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
  }

  private boolean isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
  }

  private void comment(){
	  // I want to figure out the end essentally:
	  // cases:
	  // 1 > /* something something etc c*/
	  // 2 > /* something something \n
	  //       etc etc  */
	  //3 : /* skske /*soemthing small */ ksdjkj */
	  while(!(peek()=='*' && peekNext()=='/') && !isAtEnd() ){  
		  // recursive loop maybe if we encounter another comment?
		  if (peek()=='/' && peekNext()=='*'){
			advance();
			advance();
			comment();
			continue;
		  }
		  if(peek()=='\n'){
			  line++;
		 }
		advance();
	  }
	  if(isAtEnd()){
		  Iroh.error(line,"Unterminated Comment");
		  return;
	  }
	  
		  advance();
		  advance();
	  
	  
  }
}
