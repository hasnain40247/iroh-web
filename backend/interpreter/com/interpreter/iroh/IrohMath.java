package com.interpreter.iroh;
import java.util.List;
class IrohMath extends BaseIrohObject {

  @Override
  public Object get(Token name) {
    switch (name.lexeme) {

      case "sqrt":
        return new IrohCallable() {
          public int arity() { return 1; }
          public Object call(Interpreter i, List<Object> args) {
            return Math.sqrt((double) args.get(0));
          }
        };

      case "pow":
        return new IrohCallable() {
          public int arity() { return 2; }
          public Object call(Interpreter i, List<Object> args) {
            return Math.pow((double) args.get(0), (double) args.get(1));
          }
        };

      case "abs":
        return new IrohCallable() {
          public int arity() { return 1; }
          public Object call(Interpreter i, List<Object> args) {
            return Math.abs((double) args.get(0));
          }
        };

      case "random":
        return new IrohCallable() {
          public int arity() { return 0; }
          public Object call(Interpreter i, List<Object> args) {
            return Math.random();
          }
        };

      case "floor":
        return new IrohCallable() {
          public int arity() { return 1; }
          public Object call(Interpreter i, List<Object> args) {
            return Math.floor((double) args.get(0));
          }
        };

      case "ceil":
        return new IrohCallable() {
          public int arity() { return 1; }
          public Object call(Interpreter i, List<Object> args) {
            return Math.ceil((double) args.get(0));
          }
        };

      case "PI":
        return Math.PI;

      default:
        throw new RunTimeError(name, "Undefined Math property.");
    }
  }
}
