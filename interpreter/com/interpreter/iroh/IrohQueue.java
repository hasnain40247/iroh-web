 package com.interpreter.iroh;

import java.util.ArrayList;
import java.util.List;
 class IrohQueue extends BaseIrohObject {
  final List<Object> elements = new ArrayList<>();

  @Override
  public Object get(Token name) {
    switch (name.lexeme) {

      case "enqueue":
        return new IrohCallable() {
          @Override
          public int arity() { return 1; }

          @Override
          public Object call(Interpreter interpreter, List<Object> args) {
            elements.add(args.get(0));
            return null;
          }
        };

      case "dequeue":
        return new IrohCallable() {
          @Override
          public int arity() { return 0; }

          @Override
          public Object call(Interpreter interpreter, List<Object> args) {
            if (elements.isEmpty()) return null;
            return elements.remove(0);
          }
        };

      case "size":
        return (double) elements.size();
    }

    throw new RunTimeError(name, "Undefined queue property.");
  }

  @Override
  public String toString() {
    return elements.toString();
  }
}
