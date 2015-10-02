// Copyright 2011 the Sputnik authors.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/**
* @name: S15.3.4.4_A14;
* @section: 15.3.4.4;
* @assertion: If IsCallable(func) is false, then throw a TypeError exception.
* @negative TypeError;
*/

Function.prototype.call.call(null, {});
