package com.interpreter.iroh;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

public class Iroh {
	  private static boolean hadError = false;
	  static boolean hadRuntimeError = false;
	  private static final Interpreter interpreter = new Interpreter();

  public static void main(String[] args) throws IOException {
    if (args.length > 1) {
      System.out.println("Usage: lotus  [script]");
      System.exit(64); 
    } else if (args.length == 1) {
      runFile(args[0]);
    } else {
      runPrompt();
    }
  }

  private static void runFile(String path) throws IOException{
	/*essentially reading a file here*/
	  byte[] bytes = Files.readAllBytes(Paths.get(path));
	  interpreter.replMode=false;
	  run(new String(bytes,Charset.defaultCharset()));

	  if(hadError) System.exit(65);
	   if (hadRuntimeError) System.exit(70);
  }
  private static void runPrompt() throws IOException {
	/* we gotta read off of system and run it indiv.*/
	InputStreamReader input=new InputStreamReader(System.in);
	BufferedReader reader=new BufferedReader(input);

	for(;;){
		System.out.print("> ");
		String line=reader.readLine();
		if (line==null) break;
		interpreter.replMode=true;
		run(line);
		hadError=false;
	}
  }
  private static void run(String source){
	  /*Need to implement the Scanner > it'll only scan our input and output tokens that are chunked.*/
	  Scanner scanner= new Scanner(source);
	  List<Token> tokens=scanner.scanTokens();

	  //for (Token token:tokens){

		  //System.out.println(token);
	  //}
	 Parser parser = new Parser(tokens);
   // Expr expression = parser.parse();
	List<Stmt> statements=parser.parse();
    // Stop if there was a syntax error.
    if (hadError) return;
	Resolver resolver = new Resolver(interpreter);
    resolver.resolve(statements);
	if (hadError) return;
   //ystem.out.println(new AstPrinter().print(expression));
	interpreter.interpret(statements);
  
}

static void error(int line,String message){
	report(line, "",message);
}

	private static void report(int line, String where, String message){
		System.err.println("> [line "+line+" ] Error"+ where +": "+ message);
		 hadError = true;
	}
 
 static void error(Token token, String message) {
    if (token.type == TokenType.EOF) {
      report(token.line, " at end", message);
    } else {
      report(token.line, " at '" + token.lexeme + "'", message);
    }
  }
  static void runTimeError(RunTimeError error) {
    System.err.println(error.getMessage() +
        "\n[line " + error.token.line + "]");
    hadRuntimeError = true;
  }
}
