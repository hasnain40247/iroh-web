package com.interpreter.iroh;

import java.util.HashMap;
import java.util.Map;

class IrohInstance extends BaseIrohObject {
  private IrohClass klass;
  private final Map<String, Object> fields = new HashMap<>();


  IrohInstance(IrohClass klass) {
    this.klass = klass;
  }

  @Override
  public String toString() {
    return klass.name + " instance";
  }

	@Override
  public Object get(Token name) {
    if (fields.containsKey(name.lexeme)) {
      return fields.get(name.lexeme);
    }
	IrohFunction method = klass.findMethod(name.lexeme);
	if (method != null) return method.bind(this);
			
    
    throw new RunTimeError(name,
        "Undefined property '" + name.lexeme + "'.");
  }
  @Override
  public void set(Token name, Object value) {
    fields.put(name.lexeme, value);
  }
}
