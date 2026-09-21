package com.interpreter.iroh;

class Break extends RuntimeException {
  
  Break() {
    super(null, null, false, false);
  }
}
