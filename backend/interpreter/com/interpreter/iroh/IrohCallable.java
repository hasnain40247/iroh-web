package com.interpreter.iroh;

import java.util.List;

interface IrohCallable {
	int arity();
  Object call(Interpreter interpreter, List<Object> arguments);
  String toString();
}
