package com.interpreter.iroh;
import java.util.List;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.Scanner;
class Interpreter implements Expr.Visitor<Object>, Stmt.Visitor<Object> {

	  
	  final Environment globals = new Environment();
	  private Environment environment = globals;
	  boolean replMode = false;
	  private final Map<Expr, Integer> locals = new HashMap<>();

	 Interpreter() {
		 globals.define("type", new IrohCallable() {

  @Override
  public int arity() {
    return 1;
  }

  @Override
  public Object call(Interpreter interpreter, List<Object> args) {
    Object value = args.get(0);

    if (value == null) return "nil";

    if (value instanceof Double) return "number";
    if (value instanceof Boolean) return "boolean";
    if (value instanceof String) return "string";

    if (value instanceof IrohList) return "list";
    if (value instanceof IrohMap) return "map";
    if (value instanceof IrohInstance) return "instance";
    if (value instanceof IrohClass) return "class";
    if (value instanceof IrohCallable) return "function";

    return value.getClass().getSimpleName().toLowerCase();
  }
});

		globals.define("input", new IrohCallable() {
			private final Scanner scanner = new Scanner(System.in);

			@Override
			public Object call(Interpreter interpreter, List<Object> arguments) {
	    if (!arguments.isEmpty()) {
		  System.out.print(arguments.get(0));
		}
		return scanner.nextLine();
  }

  @Override
  public int arity() {
    return 1;
  }
});


		 globals.define("clock", new IrohCallable() {
		 @Override
		 public int arity() { return 0; }

		 @Override
		 public Object call(Interpreter interpreter,
                         List<Object> arguments) {
				 return (double)System.currentTimeMillis() / 1000.0;
      }

		 @Override
		 public String toString() { return "<native fn>"; }
    });


	globals.define("Queue", new IrohCallable() {
  @Override
  public int arity() { return 0; }

  @Override
  public Object call(Interpreter interpreter, List<Object> args) {
    return new IrohQueue();
  }
});
	globals.define("Math", new IrohCallable() {
  @Override
  public int arity() { return 0; }

  @Override
  public Object call(Interpreter interpreter, List<Object> args) {
    return new IrohMath();
  }
});
globals.define("Map", new IrohCallable() {
  @Override
  public int arity() { return 0; }

  @Override
  public Object call(Interpreter interpreter, List<Object> args) {
    return new IrohMap();
  }
});
  }
	void interpret(List<Stmt> stmts){
		try{
			for(Stmt stmt:stmts){
				execute(stmt);
			}
			//System.out.println(stringify(value));
		}catch(RunTimeError error){
			Iroh.runTimeError(error);
		}
	}
	private void execute(Stmt stmt){
		stmt.accept(this);
	}

	void resolve(Expr expr, int depth) {
    locals.put(expr, depth);
	}

	private String stringify(Object object) {
    if (object == null) return "nil";

    if (object instanceof Double) {
      String text = object.toString();
      if (text.endsWith(".0")) {
        text = text.substring(0, text.length() - 2);
      }
      return text;
    }

    return object.toString();
  }
    @Override
  public Void visitVarStmt(Stmt.Var stmt) {
    Object value = null;
	// it'll come here first > 2+2 !=null > evaluate(expression)
    if (stmt.initializer != null) {
      value = evaluate(stmt.initializer);
    }

    environment.define(stmt.name.lexeme, value);
    return null;
  }

	@Override
	public Void visitBlockStmt(Stmt.Block stmt){
		// we gotta execute each statement in the block I'm sure.
		executeBlock(stmt.stmts,new Environment(environment));
		return null;
	}
	void executeBlock(List<Stmt> stmts,Environment env){
		Environment previous=this.environment;
		try{
			this.environment=env;
			for(Stmt stmt:stmts){
				execute(stmt);
			}
		}finally{
			this.environment=previous;
		}
	}



 @Override
  public Object visitVariableExpr(Expr.Variable expr) {
		//return environment.get(expr.name);
	 return lookUpVariable(expr.name, expr);
  }
  
 private Object lookUpVariable(Token name, Expr expr) {
    Integer distance = locals.get(expr);
    if (distance != null) {
      return environment.getAt(distance, name.lexeme);
    } else {
      return globals.get(name);
    }
  }

 @Override
public Object visitAssignExpr(Expr.Assign expr){
	Object value=evaluate(expr.value);
	 Integer distance = locals.get(expr);
    if (distance != null) {
      environment.assignAt(distance, expr.name, value);
    } else {
      globals.assign(expr.name, value);
    }

	return value;
}

  @Override
  public Object visitLogicalExpr(Expr.Logical expr) {
	Object left=evaluate(expr.left);
	if(expr.operator.type==TokenType.OR){
		if(isTruth(left)){
			return left;
		}
	}
	if(expr.operator.type==TokenType.AND){
		if(!isTruth(left)){
			return left; // A AND B > 1 & 1 =1 ; 0 & 1 =0
		}
	}
    return evaluate(expr.right);
  }


@Override
public Object visitIndexExpr(Expr.Index expr) {
  Object object = evaluate(expr.object);
  Object index = evaluate(expr.index);

  if (!(object instanceof IrohList list)) {
    throw new RunTimeError(expr.bracket,
        "Only lists can be indexed.");
  }

  if (!(index instanceof Double)) {
    throw new RunTimeError(expr.bracket,
        "Index must be a number.");
  }

  int i = ((Double) index).intValue();

  return list.get(i); 
}



@Override
public Object visitGetExpr(Expr.Get expr) {
  Object object = evaluate(expr.object);

  if (object instanceof IrohClass klass) {
    return klass.getStatic(expr.name);
  }

  if (object instanceof IrohObject io) {
    Object value = io.get(expr.name);

    if (value instanceof IrohFunction fn && fn.isGetter()) {
      return fn.call(this, List.of());
    }

    return value;
  }

  throw new RunTimeError(expr.name,
      "Only objects have properties.");
}


 @Override
  public Void visitSwitchStmt(Stmt.Switch stmt) {
    Object subject = evaluate(stmt.subject);
    for (Stmt.SwitchCase c : stmt.cases) {
      if (c.value == null || isEqual(subject, evaluate(c.value))) {
        try {
          for (Stmt s : c.body) execute(s);
        } catch (Break b) {}
        break;
      }
    }
    return null;
  }

 @Override
  public Void visitWhileStmt(Stmt.While stmt) {
    while(isTruth(evaluate(stmt.condition))) {
  	   try {
            execute(stmt.thenBranch);
        } catch (Break b) {
            break;
        } catch (Continue c) {}
    }
    return null;
  }


 @Override
  public Void visitIFStmt(Stmt.IF stmt) {
    if (isTruth(evaluate(stmt.condition))) {
      execute(stmt.thenBranch);
    } else if (stmt.elseBranch != null) {
      execute(stmt.elseBranch);
    }
    return null;
  }


	@Override
	public Object visitLiteralExpr(Expr.Literal expr){
	
		return expr.value;
	}

	
	@Override
	public Void visitExpressionStmt(Stmt.Expression stmt){
		Object value=evaluate(stmt.expression);
		if(replMode){
			System.out.println(stringify(value));
		}
		return null;
	}

  @Override
  public Void visitPrintStmt(Stmt.Print stmt) {
    Object value = evaluate(stmt.expression);
    System.out.println(stringify(value));
    return null;
  }
	@Override
	public Object visitGroupingExpr(Expr.Grouping expr){
		return evaluate(expr.expression);
	}

	private Object evaluate(Expr expr){
		return expr.accept(this);
	}
	public Object visitUnaryExpr(Expr.Unary expr){
		Object right=evaluate(expr.right);

		switch(expr.operator.type){
			case MINUS:
				checkNumberOperand(expr.operator,right);
				return -(double)right;
			case BANG:
				return !isTruth(right);

		}
		return null;
	}

	private void checkNumberOperand(Token operator,Object operand){
		if(operand instanceof Double)
			return;
		throw new RunTimeError(operator,"Operand must be a number");
	}
	private void checkNumberOperands(Token operator,Object left, Object right){
		if(left instanceof Double && right instanceof Double )
			return;
		throw new RunTimeError(operator,"Operand must be a number");
	}
	private Boolean isTruth(Object object){
	
		if (object==null){
			return false;
		}
		if(object instanceof Boolean){
			return (boolean)object;
		}
		return true;
	}



	public Object visitTernaryExpr(Expr.Ternary expr){
		// essentially we must return a boolean so we know that atleast or wait actually return the value of the branch itself
		// but evaluate only if the conditional goes through
		// if 1==2 > evaluate > comparison > return 
		if(isTruth(evaluate(expr.conditional))){
			return evaluate(expr.true_branch);
		}else{
			return evaluate(expr.false_branch);
		
		}
	}





	public Object visitBinaryExpr(Expr.Binary expr){
		// we essentially have this case: (expression) operator (expression) > which means we must do post order by evaluating right and left objects
		Object left=evaluate(expr.left);
		Object right=evaluate(expr.right);
		Token operator=expr.operator;
		switch(operator.type){
			case MINUS:
				checkNumberOperands(expr.operator, left, right);
				return (double)left - (double)right;
			case STAR:
				checkNumberOperands(expr.operator, left, right);
				return (double)left * (double)right;
			case SLASH:
				checkNumberOperands(expr.operator, left, right);
				checkZeroDivision(expr.operator,left,right);
				return (double)left / (double)right;
			case PLUS:
				if(left instanceof Double && right instanceof Double){
					return (double)left + (double)right;
				}
				if(left instanceof String && right instanceof String){
					return (String)left + (String)right;
				}
				if(left instanceof String || right instanceof String){
					return stringify(left) + stringify(right);
				}
				throw new RunTimeError(expr.operator,
            "Operands must be two numbers or two strings or either a number or string.");
			case LESS:
				checkNumberOperands(expr.operator, left, right);
				return (double) left < (double)right;
			case LESS_EQUAL:
				checkNumberOperands(expr.operator, left, right);
				return (double)left <= (double)right;
			case GREATER:
				checkNumberOperands(expr.operator, left, right);
				return (double)left > (double)right;
			case GREATER_EQUAL:
				checkNumberOperands(expr.operator, left, right);
				return (double)left >= (double)right;
			case BANG_EQUAL:
				return !isEqual(left,right);
			case EQUAL_EQUAL:
				return isEqual(left,right);

		}
		return null;
	}


	private Boolean isEqual(Object left,Object right){
		// != ==
		//
		// (expression) evaluator (expression)
		// this expression can either be true or false
		// evaluate(expression)
		if(left==null && right==null){
			return true;
		}
		if (left==null){
			return false;
		}
		return left.equals(right);
	}
	private void checkZeroDivision(Token operator,Object left, Object right){
		if((double)right!=0) return;

		throw new RunTimeError(operator, "ZeroDivisionError: division by zero");
		
	}
	@Override
	public Object visitCallExpr(Expr.Call call){
		Object callee=evaluate(call.callee);
		List<Object> arguments = new ArrayList<>();
		for (Expr argument : call.arguments) { 
			arguments.add(evaluate(argument));
		}
		
		if (!(callee instanceof IrohCallable)) {
			throw new RunTimeError(call.paren,
				"Can only call functions and classes.");
		}

		IrohCallable func=(IrohCallable)callee;
		if (arguments.size() != func.arity()) {
			 throw new RunTimeError(call.paren, "Expected " +
			func.arity() + " arguments but got " +
			arguments.size() + ".");
    }
		return func.call(this,arguments);
	}

	@Override
	public Void visitFunctionStmt(Stmt.Function stmt) {
		IrohFunction function = new IrohFunction(stmt,environment,false);
		environment.define(stmt.name.lexeme, function);
		return null;
  }

  @Override
public Object visitAnonFunctionExprExpr(Expr.AnonFunctionExpr expr) {
    return new IrohFunction(expr, environment,false);
}

  @Override
  public Void visitReturnStmt(Stmt.Return stmt) {
    Object value = null;
    if (stmt.value != null) value = evaluate(stmt.value);

    throw new Return(value);
  }
@Override
public Void visitBreakStmt(Stmt.Break stmt) {
    throw new Break();
}

@Override
public Void visitContinueStmt(Stmt.Continue stmt) {
    throw new Continue();
}

  @Override
  public Void visitClassStmt(Stmt.Class stmt) {
	    Object superclass = null;
    if (stmt.superclass != null) {
      superclass = evaluate(stmt.superclass);
      if (!(superclass instanceof IrohClass)) {
        throw new RunTimeError(stmt.superclass.name,
            "Superclass must be a class.");
      }
    }
    environment.define(stmt.name.lexeme, null);
	if (stmt.superclass != null) {
      environment = new Environment(environment);
      environment.define("elder", superclass);
    }
	
	Map<String, IrohFunction> methods = new HashMap<>();
	Map<String, IrohFunction> statics = new HashMap<>();
    for (Stmt.Function method : stmt.methods) {
		  IrohFunction function = new IrohFunction(
      method,
      environment,
      method.name.lexeme.equals("init")
  );

  if (method.isStatic) {
    statics.put(method.name.lexeme, function);
  } else {

    methods.put(method.name.lexeme, function);
  }
	    }

    IrohClass klass = new IrohClass(stmt.name.lexeme,  (IrohClass)superclass,methods,statics);
	 if (superclass != null) {
      environment = environment.enclosing;
    }

    environment.assign(stmt.name, klass);
    return null;
  }
  @Override
  public Object visitSuperExpr(Expr.Super expr) {
    int distance = locals.get(expr);
    IrohClass superclass = (IrohClass)environment.getAt(
        distance, "elder");
	 IrohInstance object = (IrohInstance)environment.getAt(
        distance - 1, "stem");

	  IrohFunction method = superclass.findMethod(expr.method.lexeme);

		   if (method == null) {
      throw new RunTimeError(expr.method,
          "Undefined property '" + expr.method.lexeme + "'.");
    }
    return method.bind(object);
  }
  @Override
  public Object visitSetExpr(Expr.Set expr) {

    Object object = evaluate(expr.object);

    if (!(object instanceof IrohInstance)) { 
      throw new RunTimeError(expr.name,
                             "Only instances have fields.");
    }

    Object value = evaluate(expr.value);
    ((IrohInstance)object).set(expr.name, value);
    return value;
  }
  @Override
  public Object visitThisExpr(Expr.This expr) {
    return lookUpVariable(expr.keyword, expr);
  }
@Override
public Object visitListExprExpr(Expr.ListExpr expr) {
  List<Object> list = new ArrayList<>();

  for (Expr element : expr.elements) {
	list.add(evaluate(element)); 
  }
return new IrohList(list);
  
}
private Object listGetMethod(IrohList list, String name) {
  switch (name) {
    case "push":
      return new IrohCallable() {
        @Override
        public int arity() { return 1; }

        @Override
        public Object call(Interpreter interpreter, List<Object> args) {
          list.elements.add(args.get(0));
          return null;
        }

        @Override
        public String toString() {
          return "<native fn push>";
        }
      };

    case "pop":
      return new IrohCallable() {
        @Override
        public int arity() { return 0; }

        @Override
        public Object call(Interpreter interpreter, List<Object> args) {
          if (list.elements.isEmpty()) return null;
          return list.elements.remove(list.elements.size() - 1);
        }

        @Override
        public String toString() {
          return "<native fn pop>";
        }
      };
  }

  throw new RunTimeError(null, "Undefined list method '" + name + "'.");
}
}
