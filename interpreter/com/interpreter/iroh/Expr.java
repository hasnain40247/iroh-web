package com.interpreter.iroh;

import java.util.List;

abstract class Expr {
  interface Visitor<R> {
    R visitBinaryExpr(Binary expr);
    R visitGroupingExpr(Grouping expr);
    R visitLiteralExpr(Literal expr);
    R visitUnaryExpr(Unary expr);
    R visitTernaryExpr(Ternary expr);
    R visitVariableExpr(Variable expr);
    R visitAssignExpr(Assign expr);
    R visitLogicalExpr(Logical expr);
    R visitCallExpr(Call expr);
    R visitGetExpr(Get expr);
    R visitSetExpr(Set expr);
    R visitAnonFunctionExprExpr(AnonFunctionExpr expr);
    R visitThisExpr(This expr);
    R visitSuperExpr(Super expr);
    R visitListExprExpr(ListExpr expr);
    R visitIndexExpr(Index expr);
  }

 static class Binary extends Expr {
  final Expr left;
  final Token operator;
  final Expr right;
  Binary(Expr left, Token operator, Expr right) {
   this.left=left;
   this.operator=operator;
   this.right=right;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitBinaryExpr(this);
    }
}

 static class Grouping extends Expr {
  final Expr expression;
  Grouping(Expr expression) {
   this.expression=expression;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitGroupingExpr(this);
    }
}

 static class Literal extends Expr {
  final Object value;
  Literal(Object value) {
   this.value=value;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitLiteralExpr(this);
    }
}

 static class Unary extends Expr {
  final Token operator;
  final Expr right;
  Unary(Token operator, Expr right) {
   this.operator=operator;
   this.right=right;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitUnaryExpr(this);
    }
}

 static class Ternary extends Expr {
  final Expr conditional;
  final Token operator;
  final Expr true_branch;
  final Expr false_branch;
  Ternary(Expr conditional, Token operator, Expr true_branch, Expr false_branch) {
   this.conditional=conditional;
   this.operator=operator;
   this.true_branch=true_branch;
   this.false_branch=false_branch;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitTernaryExpr(this);
    }
}

 static class Variable extends Expr {
  final Token name;
  Variable(Token name) {
   this.name=name;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitVariableExpr(this);
    }
}

 static class Assign extends Expr {
  final Token name;
  final Expr value;
  Assign(Token name, Expr value) {
   this.name=name;
   this.value=value;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitAssignExpr(this);
    }
}

 static class Logical extends Expr {
  final Expr left;
  final Token operator;
  final Expr right;
  Logical(Expr left, Token operator, Expr right) {
   this.left=left;
   this.operator=operator;
   this.right=right;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitLogicalExpr(this);
    }
}

 static class Call extends Expr {
  final Expr callee;
  final Token paren;
  final List<Expr> arguments;
  Call(Expr callee, Token paren, List<Expr> arguments) {
   this.callee=callee;
   this.paren=paren;
   this.arguments=arguments;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitCallExpr(this);
    }
}

 static class Get extends Expr {
  final Expr object;
  final Token name;
  Get(Expr object, Token name) {
   this.object=object;
   this.name=name;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitGetExpr(this);
    }
}

 static class Set extends Expr {
  final Expr object;
  final Token name;
  final Expr value;
  Set(Expr object, Token name, Expr value) {
   this.object=object;
   this.name=name;
   this.value=value;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitSetExpr(this);
    }
}

 static class AnonFunctionExpr extends Expr {
  final List<Token> params;
  final List<Stmt> body;
  AnonFunctionExpr(List<Token> params, List<Stmt> body) {
   this.params=params;
   this.body=body;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitAnonFunctionExprExpr(this);
    }
}

 static class This extends Expr {
  final Token keyword;
  This(Token keyword) {
   this.keyword=keyword;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitThisExpr(this);
    }
}

 static class Super extends Expr {
  final Token keyword;
  final Token method;
  Super(Token keyword, Token method) {
   this.keyword=keyword;
   this.method=method;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitSuperExpr(this);
    }
}

 static class ListExpr extends Expr {
  final List<Expr> elements;
  ListExpr(List<Expr> elements) {
   this.elements=elements;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitListExprExpr(this);
    }
}

 static class Index extends Expr {
  final Expr object;
  final Token bracket;
  final Expr index;
  Index(Expr object, Token bracket, Expr index) {
   this.object=object;
   this.bracket=bracket;
   this.index=index;
  }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitIndexExpr(this);
    }
}

  abstract <R> R accept(Visitor<R> visitor);
}
