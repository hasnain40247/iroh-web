package com.interpreter.iroh;

import java.util.List;
import java.util.ArrayList;

abstract class Stmt {
  interface Visitor<R> {
    R visitExpressionStmt(Expression stmt);
    R visitPrintStmt(Print stmt);
    R visitVarStmt(Var stmt);
	R visitBlockStmt(Block stmt);
	R visitIFStmt(IF stmt);
	R visitWhileStmt(While stmt);
	R visitFunctionStmt(Function stmt);
	R visitReturnStmt(Return stmt);
	R visitBreakStmt(Break stmt);
	R visitContinueStmt(Continue stmt);
	R visitClassStmt(Class stmt);
	R visitSwitchStmt(Switch stmt);
  }

  static class SwitchCase {
    final Expr value; // null for default
    final List<Stmt> body;
    SwitchCase(Expr value, List<Stmt> body) {
      this.value = value;
      this.body = body;
    }
  }

  static class Switch extends Stmt {
    final Expr subject;
    final List<SwitchCase> cases;
    Switch(Expr subject, List<SwitchCase> cases) {
      this.subject = subject;
      this.cases = cases;
    }
    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitSwitchStmt(this);
    }
  }
  static class Break extends Stmt{
	final Token keyword;
	Break(Token keyword){
		this.keyword=keyword;
	}
	@Override
	<R> R accept(Visitor<R> visitor){
		return visitor.visitBreakStmt(this);
	}
  }


  static class Class extends Stmt{
	final Token name;
	final Expr.Variable superclass;
	final List<Stmt.Function> methods;

	Class(Token name,Expr.Variable superclass,List<Stmt.Function> methods){
		this.name=name;
		this.superclass=superclass;
		this.methods=methods;
	}
	@Override
	<R> R accept(Visitor<R> visitor){
		return visitor.visitClassStmt(this);
	}
  }







  static class Continue extends Stmt{
	final Token keyword;
	Continue(Token keyword){
		this.keyword=keyword;
	}
	@Override
	<R> R accept(Visitor<R> visitor){
		return visitor.visitContinueStmt(this);
	}
  }

  static class Return extends Stmt{
	final Token keyword;
	final Expr value;
	Return(Token keyword, Expr value){
		this.value=value;
		this.keyword=keyword;

	}
	@Override
	<R> R accept(Visitor<R> visitor){
		return visitor.visitReturnStmt(this);
	}
  }
 
  
   static class Function extends Stmt{
	final Token name;
	final List<Token> params;
	final List<Stmt> body;
	final boolean isStatic;
	final boolean isGetter;
	Function(Token name,List<Token> params,List<Stmt> body,boolean isStatic,boolean isGetter){
		this.name=name;
		this.isGetter=isGetter;
		this.isStatic=isStatic;
		this.params=params;
		this.body=body;
	}
	@Override
	<R> R accept(Visitor<R> visitor){
		return visitor.visitFunctionStmt(this);
	}
  }
 

  static class Block extends Stmt{
	final List<Stmt> stmts;
	Block(List<Stmt> stmts){
		this.stmts=stmts;
	}
	@Override
	<R> R accept(Visitor<R> visitor){
		return visitor.visitBlockStmt(this);
	}
  }

	static class While extends Stmt {
		final Expr condition;
		final Stmt thenBranch;

		While(Expr condition, Stmt thenBranch){
			this.condition=condition;
			this.thenBranch=thenBranch;
		}
		@Override
		<R> R accept(Visitor<R> visitor){
			return visitor.visitWhileStmt(this);
		}
	}





static class IF extends Stmt {

	final Expr condition;
	final Stmt thenBranch;
	final Stmt elseBranch;
	IF(Expr condition,Stmt thenBranch,Stmt elseBranch){
		this.condition=condition;
		this.thenBranch=thenBranch;
		this.elseBranch=elseBranch;
	}

	@Override
	<R> R accept(Visitor<R> visitor){
		return visitor.visitIFStmt(this);
	}


}
 static class Expression extends Stmt {
  final Expr expression;
  Expression(Expr expression) {
   this.expression=expression;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitExpressionStmt(this);
    }
}

 static class Print extends Stmt {
  final Expr expression;
  Print(Expr expression) {
   this.expression=expression;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitPrintStmt(this);
    }
}

 static class Var extends Stmt {
  final Token name;
  final Expr initializer;
  Var(Token name, Expr initializer) {
   this.name=name;
   this.initializer=initializer;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitVarStmt(this);
    }
}

  abstract <R> R accept(Visitor<R> visitor);
}
