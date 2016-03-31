// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// in escargot, the number of function call arguments is limited to 65535. so sae-bom.kim changed the code to 10000.
//var x = Array(100000);
var x = Array(10000);
y =  Array.apply(Array, x);
y.unshift(4);
y.shift();
