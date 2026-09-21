package com.interpreter.iroh;

import java.util.ArrayList;
import java.util.List;


class IrohList extends BaseIrohObject {
  final List<Object> elements = new ArrayList<>();

  IrohList(List<Object> initial) {
    elements.addAll(initial);
  }
public Object get(int index) {
  if (index < 0 || index >= elements.size()) {
    throw new RunTimeError(null, "Index out of bounds.");
  }
  return elements.get(index);
}
  @Override
  public Object get(Token name) {
    switch (name.lexeme) {
      case "size":
        return (double) elements.size();

	case "contains":
  return new IrohCallable() {
    @Override
    public int arity() { return 1; }

    @Override
    public Object call(Interpreter interpreter, List<Object> args) {
      return elements.contains(args.get(0));
    }
  };
  case "index":
  return new IrohCallable() {
    @Override
    public int arity() { return 1; }

    @Override
    public Object call(Interpreter interpreter, List<Object> args) {
      return (double) elements.indexOf(args.get(0));
    }
  };
  case "slice":
  return new IrohCallable() {
    @Override
    public int arity() { return 2; }

    @Override
    public Object call(Interpreter interpreter, List<Object> args) {
      int start = ((Double) args.get(0)).intValue();
      int end = ((Double) args.get(1)).intValue();

      start = Math.max(0, start);
      end = Math.min(elements.size(), end);

      return new IrohList(new ArrayList<>(elements.subList(start, end)));
    }
  };

      case "push":
        return new IrohCallable() {
          @Override
          public int arity() { return 1; }

          @Override
          public Object call(Interpreter interpreter, List<Object> args) {
            elements.add(args.get(0));
            return null;
          }
        };

      case "pop":
        return new IrohCallable() {
          @Override
          public int arity() { return 0; }

          @Override
          public Object call(Interpreter interpreter, List<Object> args) {
            if (elements.isEmpty()) return null;
            return elements.remove(elements.size() - 1);
          }
        };
    }

    throw new RunTimeError(name, "Undefined list property.");
  }

  @Override
  public String toString() {
    return elements.toString();
  }
}
