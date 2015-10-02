// Copyright 2009 the Sputnik authors.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/**
 * @name: S15.1_A1_T1;
 * @section: 15.1;
 * @assertion: The global object does not have a [[Construct]] property;
 * @description: It is not possible to use the global object as a constructor 
 * with the new operator;
 * @negative  
*/

new this;
