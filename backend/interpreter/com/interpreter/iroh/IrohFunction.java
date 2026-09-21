package com.interpreter.iroh;

import java.util.List;
class IrohFunction implements IrohCallable {
  private final List<Token> params;
  private final List<Stmt> body;
  private final String name;
  private final Environment closure;
  private final boolean isInitializer;
  private final Stmt.Function declaration;
private final Expr.AnonFunctionExpr anonDeclaration;
private final boolean isStatic;
private final boolean isGetter;

  IrohFunction(Stmt.Function declaration, Environment closure,boolean isInitializer) {
	  this.isInitializer = isInitializer;
	  this.declaration = declaration;
  this.anonDeclaration = null;
    this.params = declaration.params;
    this.body = declaration.body;
    this.name = declaration.name.lexeme;
    this.closure = closure;
	this.isGetter = declaration.isGetter;
 
	this.isStatic = declaration.isStatic;
  }

  IrohFunction(Expr.AnonFunctionExpr expr, Environment closure,boolean isInitializer) {
	  this.declaration = null;
  this.anonDeclaration = expr;
    this.params = expr.params;
    this.body = expr.body;
    this.name = null;
    this.closure = closure;
this.isInitializer = isInitializer;
	this.isGetter = false;
	this.isStatic = false;
  }

  @Override
  public Object call(Interpreter interpreter, List<Object> arguments) {
    Environment env = new Environment(closure);
    for (int i = 0; i < params.size(); i++) {
      env.define(params.get(i).lexeme, arguments.get(i));
    }
    try {
      interpreter.executeBlock(body, env);
    } catch (Return returnValue) {
		      if (isInitializer) return closure.getAt(0, "stem");

      return returnValue.value;
    }
	if (isInitializer) return closure.getAt(0, "stem");
    return null;
  }

  @Override
  public int arity() {
    return params.size();
  }

  @Override
  public String toString() {
    if (name == null) return "<fn anonymous>";
    return "<fn " + name + ">";
  }
 IrohFunction bind(IrohInstance instance) {
    Environment environment = new Environment(closure);
    environment.define("stem", instance);
	  if (declaration != null) {
    return new IrohFunction(declaration, environment,isInitializer);
  } else {
    return new IrohFunction(anonDeclaration, environment,isInitializer);
  }
  }
 public boolean isGetter() {
  return isGetter;
}
}
