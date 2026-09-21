package com.interpreter.iroh;
import static com.interpreter.iroh.TokenType.*;

import java.util.HashMap;
import java.util.Map;
class Environment {
	private final Map<String,Object> values = new HashMap<>();
	Environment enclosing; // Outer env > Inner env

	Environment(){
		enclosing=null;
	}
	Environment(Environment enclosing){
		this.enclosing=enclosing;
	}


	void define(String name, Object value){
		values.put(name,value);
	
	}

	Object get(Token name){
		if(values.containsKey(name.lexeme)){
			Object value=values.get(name.lexeme);
			if (value==null){
			
				throw new RunTimeError(name, "Variable has no value assigned "+name.lexeme+".");
			}
			return value;
		}
		// we need to walk to the inner most environment and get the name:
		if(enclosing!=null){
			return enclosing.get(name);
		}
		throw new RunTimeError(name,"Undefined variable " + name.lexeme+"." );
	
	}
	void assign(Token name,Object value){
	if(values.containsKey(name.lexeme)){
			values.put(name.lexeme,value);return;
		}
	// if the env is not in this environment then walk the chain to find the scope when its defined?
	if(enclosing!=null){
		enclosing.assign(name,value);
		return;
	}
		throw new RunTimeError(name,"Undefined variable " + name.lexeme+"." );

	}

	 Object getAt(int distance, String name) {
    return ancestor(distance).values.get(name);
  }

  Environment ancestor(int distance) {
    Environment environment = this;
    for (int i = 0; i < distance; i++) {
      environment = environment.enclosing;
    }

    return environment;
  }

   void assignAt(int distance, Token name, Object value) {
    ancestor(distance).values.put(name.lexeme, value);
  }
}
