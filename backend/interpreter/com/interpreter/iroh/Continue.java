package com.interpreter.iroh;

class Continue extends RuntimeException {
  
  Continue() {
    super(null, null, false, false);
  }
}
