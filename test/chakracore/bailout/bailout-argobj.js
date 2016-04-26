//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo(a) {
       arguments[0] = "Changed";
       WScript.Echo("Arguments : " + arguments[0]);
}
foo("Orig");

function foo2(a) {
    for (var i = 0; i < 1; i++) {
       arguments[0] = "Changed";
       // Bailout point
       WScript.Echo("Arguments : " + arguments[0]);
    }
}
foo2("Orig");
