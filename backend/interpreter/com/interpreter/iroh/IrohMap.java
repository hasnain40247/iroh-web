 package com.interpreter.iroh;
import java.util.HashMap;
import java.util.Map;
import java.util.List;
import java.util.ArrayList;
class IrohMap extends BaseIrohObject {

  private final Map<Object, Object> values = new HashMap<>();

  @Override
  public Object get(Token name) {
    switch (name.lexeme) {

      case "get":
        return new IrohCallable() {
          @Override
          public int arity() { return 1; }

          @Override
          public Object call(Interpreter interpreter, List<Object> args) {
            return values.getOrDefault(args.get(0), null);
          }
        };

      case "set":
        return new IrohCallable() {
          @Override
          public int arity() { return 2; }

          @Override
          public Object call(Interpreter interpreter, List<Object> args) {
            values.put(args.get(0), args.get(1));
            return null;
          }
        };

      case "contains":
        return new IrohCallable() {
          @Override
          public int arity() { return 1; }

          @Override
          public Object call(Interpreter interpreter, List<Object> args) {
            return values.containsKey(args.get(0));
          }
        };

      case "size":
        return (double) values.size();

      case "keys":
        return new IrohCallable() {
          @Override
          public int arity() { return 0; }

          @Override
          public Object call(Interpreter interpreter, List<Object> args) {
            return new ArrayList<>(values.keySet());
          }
        };

      case "values":
        return new IrohCallable() {
          @Override
          public int arity() { return 0; }

          @Override
          public Object call(Interpreter interpreter, List<Object> args) {
            return new ArrayList<>(values.values());
          }
        };
    }

    throw new RunTimeError(name, "Undefined map property.");
  }

  @Override
  public String toString() {
    return values.toString();
  }
}
