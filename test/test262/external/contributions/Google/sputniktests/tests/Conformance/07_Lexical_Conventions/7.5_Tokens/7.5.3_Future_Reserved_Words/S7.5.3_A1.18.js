// Copyright 2009 the Sputnik authors.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/**
 * @name: S7.5.3_A1.18;
 * @section: 7.5.3;
 * @assertion: The "interface" token can not be used as identifier in
 *             strict code;
 * @description: Checking if execution of "interface = 1" fails in
 *               strict code;
 * @negative
 */

"use strict";
interface = 1;
