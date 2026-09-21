package com.interpreter.iroh;
interface IrohObject {
  Object get(Token name);
  void set(Token name, Object value);
}
