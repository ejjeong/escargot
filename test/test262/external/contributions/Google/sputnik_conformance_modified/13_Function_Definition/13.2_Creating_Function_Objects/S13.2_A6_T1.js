// Copyright 2011 Google Inc.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/**
 * @description check if "caller" poisoning poisons
 * getOwnPropertyDescriptor too
 * @onlyStrict
 */
"use strict";
Object.getOwnPropertyDescriptor(function(){}, 'caller');
