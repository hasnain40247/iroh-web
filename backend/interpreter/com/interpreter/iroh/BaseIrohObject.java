package com.interpreter.iroh;
abstract class BaseIrohObject implements IrohObject {

  @Override
  public void set(Token name, Object value) {
    throw new RunTimeError(name, "This object is not mutable.");
  }
}
