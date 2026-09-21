package com.interpreter.tool;

import java.util.Arrays;
import java.util.List;
import java.io.PrintWriter;
import java.io.IOException;

public class GenerateAst{
	public static void main(String[] args) throws IOException {
		if(args.length!=1){
			 System.err.println("Usage: generate_ast <output directory>");
			System.exit(64);
		}
		String outputDir=args[0];
		defineAst(outputDir, "Expr", Arrays.asList(
  "Binary   : Expr left, Token operator, Expr right",
  "Grouping : Expr expression",
  "Literal  : Object value",
  "Unary    : Token operator, Expr right",
  "Ternary  : Expr conditional, Token operator, Expr true_branch, Expr false_branch",
  "Variable : Token name",
  "Assign   : Token name, Expr value",
  "Logical  : Expr left, Token operator, Expr right",
  "Call     : Expr callee, Token paren, List<Expr> arguments",
  "Get      : Expr object, Token name",
  "Set      : Expr object, Token name, Expr value",
  "AnonFunctionExpr : List<Token> params, List<Stmt> body",
  "This     : Token keyword",
  "Super    : Token keyword, Token method",
  "ListExpr : List<Expr> elements",
  "Index    : Expr object, Token bracket, Expr index"
));
	}

	private static void defineAst(String outputDir,String base,List<String> types) throws IOException{

		// need to initialize printwriter with the output directory:
		String path=outputDir+"/"+base+".java";
		PrintWriter writer=new PrintWriter(path,"UTF-8");
		writer.println("package com.interpreter.iroh;");
		writer.println();
		writer.println("import java.util.List;");
		writer.println();
		writer.println("abstract class " + base + " {");
		defineVisitor(writer, base, types);

		for(String type: types){
			String className=type.split(":")[0].trim();
			String fieldNames=type.split(":")[1].trim();
			defineType(writer,base,className,fieldNames);
			
		}
		 writer.println();
	    writer.println("  abstract <R> R accept(Visitor<R> visitor);");

		writer.println("}");
		writer.close();

	}
	private static void defineType(PrintWriter writer,String base, String className, String fieldNames){
	
		writer.println();
		writer.println(" static class "+className+" extends "+base+ " {");
		String[] fields=fieldNames.split(", ");
		for(String field:fields){
		writer.println("  final " +field+ ";");
		}
		writer.println("  "+className+"("+ fieldNames+") {");
		for(String field:fields){
			String f=field.split(" ")[1];
			writer.println("   this."+f+"=" +f+ ";");
		}
		writer.println("  }");
		writer.println();
    writer.println("    @Override");
    writer.println("    <R> R accept(Visitor<R> visitor) {");
    writer.println("      return visitor.visit" +
        className + base + "(this);");
    writer.println("    }");
		writer.println("}");

	}
	 private static void defineVisitor(
      PrintWriter writer, String baseName, List<String> types) {
    writer.println("  interface Visitor<R> {");

    for (String type : types) {
      String typeName = type.split(":")[0].trim();
      writer.println("    R visit" + typeName + baseName + "(" +
          typeName + " " + baseName.toLowerCase() + ");");
    }

    writer.println("  }");
  }
}
