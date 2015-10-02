// Copyright 2009 the Sputnik authors.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/**
 * Function declaration within an "if" statement in strict code is not allowed;
 *
 * @description Declaring function within an "if" that is declared
 * within the strict function
 * @negative SyntaxError
 * @onlyStrict
 * @bestPractice
 * http://wiki.ecmascript.org/doku.php?id=conventions:no_non_standard_strict_decls
 */
"use strict";
(function(){
   if (true) {
     function __func(){};
   } else {
     function __func(){};
   }
});
