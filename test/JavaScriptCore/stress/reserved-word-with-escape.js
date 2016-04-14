function testSyntax(script) {
    try {
        eval(script);
    } catch (error) {
        if (error instanceof SyntaxError)
            throw new Error("Bad error: " + String(error));
    }
}

function testSyntaxError(script, message) {
    var error = null;
    try {
        eval(script);
    } catch (e) {
        error = e;
    }
    if (!error)
        throw new Error("Expected syntax error not thrown");

    if (String(error) !== message)
        throw new Error("Bad error: " + String(error));
}

testSyntax("var cocoa");
testSyntax("var c\u006fcoa");

testSyntaxError("var var", "SyntaxError: Cannot use the keyword 'var' as a variable name.");

testSyntaxError("var v\u0061r", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name.");

testSyntaxError("var v\u{0061}r", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name.");

testSyntaxError("var var = 2000000;", "SyntaxError: Cannot use the keyword 'var' as a variable name.");
testSyntaxError("var v\u0061r = 2000000;", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name.");
testSyntaxError("var v\u{0061}r = 2000000", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name.");

testSyntaxError("var {var} = obj)", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("var {v\u0061r} = obj", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("var {v\u{0061}r} = obj", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");

testSyntaxError("var {var:var} = obj)", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("var {var:v\u0061r} = obj", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("var {var:v\u{0061}r} = obj", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");

testSyntaxError("var [var] = obj", "SyntaxError: Unexpected Punctuator '['. Expect 'Identifier'.");
testSyntaxError("var [v\u0061r] = obj", "SyntaxError: Unexpected Punctuator '['. Expect 'Identifier'.");
testSyntaxError("var [v\u{0061}r] = obj", "SyntaxError: Unexpected Punctuator '['. Expect 'Identifier'.");

testSyntaxError("[var] = obj", "SyntaxError: Unexpected Keyword 'var'.");
testSyntaxError("[v\u0061r] = obj", "SyntaxError: Unexpected Keyword 'v\u0061r'.");
testSyntaxError("[v\u{0061}r] = obj", "SyntaxError: Unexpected Keyword 'v\u{0061}r'.");

testSyntaxError("function var() { }", "SyntaxError: Cannot use the keyword 'var' as a function name.");
testSyntaxError("function v\u0061r() { }", "SyntaxError: Cannot use the keyword 'v\u0061r' as a function name.");
testSyntaxError("function v\u{0061}r() { }", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a function name.");

testSyntaxError("function a(var) { }", "SyntaxError: Cannot use the keyword 'var' as a variable name.");
testSyntaxError("function a(v\u0061r) { }", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name.");
testSyntaxError("function a(v\u{0061}r) { }", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name.");

testSyntaxError("function a({var}) { }", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("function a({v\u0061r}) { }", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("function a({v\u{0061}r}) { }", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");

testSyntaxError("function a({var:var}) { }", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("function a({var:v\u0061r}) { }", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("function a({var:v\u{0061}r}) { }", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");

testSyntaxError("function a([var]) { }", "SyntaxError: Unexpected Punctuator '['. Expect 'Identifier'.");
testSyntaxError("function a([v\u0061r]) { }", "SyntaxError: Unexpected Punctuator '['. Expect 'Identifier'.");
testSyntaxError("function a([v\u{0061}r]) { }", "SyntaxError: Unexpected Punctuator '['. Expect 'Identifier'.");

testSyntaxError("(function var() { })", "SyntaxError: Cannot use the keyword 'var' as a function name.");
testSyntaxError("(function v\u0061r() { })", "SyntaxError: Cannot use the keyword 'v\u0061r' as a function name.");
testSyntaxError("(function v\u{0061}r() { })", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a function name.");

testSyntaxError("(function a(var) { })", "SyntaxError: Cannot use the keyword 'var' as a variable name.");
testSyntaxError("(function a(v\u0061r) { })", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name.");
testSyntaxError("(function a(v\u{0061}r) { })", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name.");

testSyntaxError("(function a({var}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("(function a({v\u0061r}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("(function a({v\u{0061}r}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");

testSyntaxError("(function a({var:var}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("(function a({var:v\u0061r}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("(function a({var:v\u{0061}r}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");

testSyntaxError("(function a([var]) { })", "SyntaxError: Unexpected Punctuator '['. Expect 'Identifier'.");
testSyntaxError("(function a([v\u0061r]) { })", "SyntaxError: Unexpected Punctuator '['. Expect 'Identifier'.");
testSyntaxError("(function a([v\u{0061}r]) { })", "SyntaxError: Unexpected Punctuator '['. Expect 'Identifier'.");

testSyntaxError("(function a([{var}]) { })", "SyntaxError: Unexpected Punctuator '['. Expect 'Identifier'.");
testSyntaxError("(function a([{v\u0061r}]) { })", "SyntaxError: Unexpected Punctuator '['. Expect 'Identifier'.");
testSyntaxError("(function a([{v\u{0061}r}]) { })", "SyntaxError: Unexpected Punctuator '['. Expect 'Identifier'.");

testSyntaxError("(function a([{var:var}]) { })", "SyntaxError: Unexpected Punctuator '['. Expect 'Identifier'.");
testSyntaxError("(function a([{var:v\u0061r}]) { })", "SyntaxError: Unexpected Punctuator '['. Expect 'Identifier'.");
testSyntaxError("(function a([{var:v\u{0061}r}]) { })", "SyntaxError: Unexpected Punctuator '['. Expect 'Identifier'.");

testSyntaxError("(function a([[var]]) { })", "SyntaxError: Unexpected Punctuator '['. Expect 'Identifier'.");
testSyntaxError("(function a([[v\u0061r]]) { })", "SyntaxError: Unexpected Punctuator '['. Expect 'Identifier'.");
testSyntaxError("(function a([[v\u{0061}r]]) { })", "SyntaxError: Unexpected Punctuator '['. Expect 'Identifier'.");

testSyntaxError("(function a({ hello: {var}}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("(function a({ hello: {v\u0061r}}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("(function a({ hello: {v\u{0061}r}}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");

testSyntaxError("(function a({ hello: {var:var}}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("(function a({ hello: {var:v\u0061r}}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("(function a({ hello: {var:v\u{0061}r}}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");

testSyntaxError("(function a({ hello: [var]}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("(function a({ hello: [v\u0061r]}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("(function a({ hello: [v\u{0061}r]}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");

testSyntaxError("(function a({ 0: {var} }) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("(function a({ 0: {v\u0061r}}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("(function a({ 0: {v\u{0061}r}}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");

testSyntaxError("(function a({ 0: {var:var}}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("(function a({ 0: {var:v\u0061r}}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("(function a({ 0: {var:v\u{0061}r}}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");

testSyntaxError("(function a({ 0: {value:var}}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("(function a({ 0: {value:v\u0061r}}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("(function a({ 0: {value:v\u{0061}r}}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");

testSyntaxError("(function a({ 0: [var]}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("(function a({ 0: [v\u0061r]}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");
testSyntaxError("(function a({ 0: [v\u{0061}r]}) { })", "SyntaxError: Unexpected Punctuator '{'. Expect 'Identifier'.");

testSyntaxError("try { } catch(var) { }", "SyntaxError: Cannot use the keyword 'var' as a catch variable name.");
testSyntaxError("try { } catch(v\u0061r) { }", "SyntaxError: Cannot use the keyword 'v\u0061r' as a catch variable name.");
testSyntaxError("try { } catch(v\u{0061}r) { }", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a catch variable name.");

testSyntaxError("class var { }", "SyntaxError: ES2015 class feature is not supported");
testSyntaxError("class v\u0061r { }", "SyntaxError: ES2015 class feature is not supported");
testSyntaxError("class v\u{0061}r { }", "SyntaxError: ES2015 class feature is not supported");

// Allowed in non-keyword aware context.
testSyntax("({ v\u0061r: 'Cocoa' })");
testSyntax("({ v\u{0061}r: 'Cocoa' })");
