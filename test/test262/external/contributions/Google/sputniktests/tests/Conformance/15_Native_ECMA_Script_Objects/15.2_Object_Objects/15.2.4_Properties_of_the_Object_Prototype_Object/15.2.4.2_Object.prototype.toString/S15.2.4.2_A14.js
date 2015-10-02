// Copyright 2011 the Sputnik authors.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/**
* @name: S15.2.4.2_A14;
* @section: 15.2.4.2;
* @assertion: Let O be the result of calling ToObject passing the this
*             value as the argument.
*/

if (Object.prototype.toString.call(33) !== "[object Number]") {
  $ERROR('Let O be the result of calling ToObject passing the this ' +
         'value as the argument.');
}
