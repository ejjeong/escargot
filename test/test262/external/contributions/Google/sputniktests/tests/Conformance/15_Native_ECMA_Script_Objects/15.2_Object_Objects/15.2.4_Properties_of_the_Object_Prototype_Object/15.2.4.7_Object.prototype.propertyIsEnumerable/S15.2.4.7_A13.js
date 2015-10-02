// Copyright 2011 the Sputnik authors.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/**
* @name: S15.2.4.7_A13;
* @section: 15.2.4.7;
* @assertion: Let O be the result of calling ToObject passing the this
*             value as the argument.
* @negative
*/

Object.prototype.propertyIsEnumerable.call(null, 'foo');
