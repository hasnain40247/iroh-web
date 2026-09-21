package com.interpreter.iroh;

import java.util.List;
import java.util.Map;

class IrohClass implements IrohCallable{
  final String name;
    private final Map<String, IrohFunction> methods;
 private final Map<String, IrohFunction> statics;
  final IrohClass superclass;

  IrohClass(String name,IrohClass superclass, Map<String, IrohFunction> methods,Map<String,IrohFunction> statics) {
    this.name = name;
    this.methods = methods;
	this.statics=statics;
	this.superclass=superclass;
  }

public Object getStatic(Token name) {
  if (statics.containsKey(name.lexeme)) {
    return statics.get(name.lexeme);
  }

  throw new RunTimeError(name,"Undefined static method");
}

  @Override
  public String toString() {
    return name;
  }

    @Override
  public Object call(Interpreter interpreter,
                     List<Object> arguments) {
    IrohInstance instance = new IrohInstance(this);
	IrohFunction initializer = findMethod("init");
    if (initializer != null) {
      initializer.bind(instance).call(interpreter, arguments);
    }

    return instance;
  }

  @Override
  public int arity() {
    IrohFunction initializer=findMethod("init");
	if(initializer==null){
		return 0;
	}
	return initializer.arity();
  }

	IrohFunction findMethod(String name) {
    if (methods.containsKey(name)) {
      return methods.get(name);
    
}
  if (superclass != null) {
      return superclass.findMethod(name);
    }

    return null;
  }
}
