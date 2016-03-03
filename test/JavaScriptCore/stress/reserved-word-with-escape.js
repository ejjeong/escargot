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

testSyntaxError("var var", "SyntaxError: Cannot use the keyword 'var' as a variable name. (Parse Error 1 line)");
testSyntaxError("var v\u0061r", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name. (Parse Error 1 line)");
testSyntaxError("var v\u{0061}r", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name. (Parse Error 1 line)");

testSyntaxError("var var = 2000000;", "SyntaxError: Cannot use the keyword 'var' as a variable name. (Parse Error 1 line)");
testSyntaxError("var v\u0061r = 2000000;", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name. (Parse Error 1 line)");
testSyntaxError("var v\u{0061}r = 2000000", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name. (Parse Error 1 line)");

//testSyntaxError("var {var} = obj)", "SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'var'.");
//testSyntaxError("var {v\u0061r} = obj", "SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'var'.");
//testSyntaxError("var {v\u{0061}r} = obj", "SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'var'.");

testSyntaxError("var {var:var} = obj)", "SyntaxError: Cannot use the keyword 'var' as a variable name. (Parse Error 1 line)");
testSyntaxError("var {var:v\u0061r} = obj", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name. (Parse Error 1 line)");
testSyntaxError("var {var:v\u{0061}r} = obj", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name. (Parse Error 1 line)");

testSyntaxError("var [var] = obj", "SyntaxError: Cannot use the keyword 'var' as a variable name. (Parse Error 1 line)");
testSyntaxError("var [v\u0061r] = obj", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name. (Parse Error 1 line)");
testSyntaxError("var [v\u{0061}r] = obj", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name. (Parse Error 1 line)");

testSyntaxError("[var] = obj", "SyntaxError: Unexpected keyword 'var' (Parse Error 1 line)");
testSyntaxError("[v\u0061r] = obj", "SyntaxError: Unexpected keyword 'v\u0061r' (Parse Error 1 line)");
testSyntaxError("[v\u{0061}r] = obj", "SyntaxError: Unexpected keyword 'v\u{0061}r' (Parse Error 1 line)");

testSyntaxError("function var() { }", "SyntaxError: Cannot use the keyword 'var' as a function name. (Parse Error 1 line)");
testSyntaxError("function v\u0061r() { }", "SyntaxError: Cannot use the keyword 'v\u0061r' as a function name. (Parse Error 1 line)");
testSyntaxError("function v\u{0061}r() { }", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a function name. (Parse Error 1 line)");

testSyntaxError("function a(var) { }", "SyntaxError: Cannot use the keyword 'var' as a variable name. (Parse Error 1 line)");
testSyntaxError("function a(v\u0061r) { }", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name. (Parse Error 1 line)");
testSyntaxError("function a(v\u{0061}r) { }", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name. (Parse Error 1 line)");

// testSyntaxError("function a({var}) { }", "SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'var'.");
// testSyntaxError("function a({v\u0061r}) { }", "SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'var'.");
// testSyntaxError("function a({v\u{0061}r}) { }", "SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'var'.");

testSyntaxError("function a({var:var}) { }", "SyntaxError: Cannot use the keyword 'var' as a variable name. (Parse Error 1 line)");
testSyntaxError("function a({var:v\u0061r}) { }", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name. (Parse Error 1 line)");
testSyntaxError("function a({var:v\u{0061}r}) { }", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name. (Parse Error 1 line)");

testSyntaxError("function a([var]) { }", "SyntaxError: Cannot use the keyword 'var' as a variable name. (Parse Error 1 line)");
testSyntaxError("function a([v\u0061r]) { }", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name. (Parse Error 1 line)");
testSyntaxError("function a([v\u{0061}r]) { }", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name. (Parse Error 1 line)")

testSyntaxError("(function var() { })", "SyntaxError: Cannot use the keyword 'var' as a function name. (Parse Error 1 line)");
testSyntaxError("(function v\u0061r() { })", "SyntaxError: Cannot use the keyword 'v\u0061r' as a function name. (Parse Error 1 line)");
testSyntaxError("(function v\u{0061}r() { })", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a function name. (Parse Error 1 line)");

testSyntaxError("(function a(var) { })", "SyntaxError: Cannot use the keyword 'var' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a(v\u0061r) { })", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a(v\u{0061}r) { })", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name. (Parse Error 1 line)");

// testSyntaxError("(function a({var}) { })", "SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'var'.");
// testSyntaxError("(function a({v\u0061r}) { })", "SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'var'.");
// testSyntaxError("(function a({v\u{0061}r}) { })", "SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'var'.");

testSyntaxError("(function a({var:var}) { })", "SyntaxError: Cannot use the keyword 'var' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a({var:v\u0061r}) { })", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a({var:v\u{0061}r}) { })", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name. (Parse Error 1 line)");

testSyntaxError("(function a([var]) { })", "SyntaxError: Cannot use the keyword 'var' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a([v\u0061r]) { })", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a([v\u{0061}r]) { })", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name. (Parse Error 1 line)");

// testSyntaxError("(function a([{var}]) { })", "SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'var'.");
// testSyntaxError("(function a([{v\u0061r}]) { })", "SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'var'.");
// testSyntaxError("(function a([{v\u{0061}r}]) { })", "SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'var'.");

testSyntaxError("(function a([{var:var}]) { })", "SyntaxError: Cannot use the keyword 'var' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a([{var:v\u0061r}]) { })", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a([{var:v\u{0061}r}]) { })", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name. (Parse Error 1 line)");

testSyntaxError("(function a([[var]]) { })", "SyntaxError: Cannot use the keyword 'var' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a([[v\u0061r]]) { })", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a([[v\u{0061}r]]) { })", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name. (Parse Error 1 line)");

// testSyntaxError("(function a({ hello: {var}}) { })", "SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'var'.");
// testSyntaxError("(function a({ hello: {v\u0061r}}) { })", "SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'var'.");
// testSyntaxError("(function a({ hello: {v\u{0061}r}}) { })", "SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'var'.");

testSyntaxError("(function a({ hello: {var:var}}) { })", "SyntaxError: Cannot use the keyword 'var' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a({ hello: {var:v\u0061r}}) { })", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a({ hello: {var:v\u{0061}r}}) { })", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name. (Parse Error 1 line)");

testSyntaxError("(function a({ hello: [var]}) { })", "SyntaxError: Cannot use the keyword 'var' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a({ hello: [v\u0061r]}) { })", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a({ hello: [v\u{0061}r]}) { })", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name. (Parse Error 1 line)");

// testSyntaxError("(function a({ 0: {var} }) { })", "SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'var'.");
// testSyntaxError("(function a({ 0: {v\u0061r}}) { })", "SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'var'.");
// testSyntaxError("(function a({ 0: {v\u{0061}r}}) { })", "SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'var'.");

testSyntaxError("(function a({ 0: {var:var}}) { })", "SyntaxError: Cannot use the keyword 'var' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a({ 0: {var:v\u0061r}}) { })", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a({ 0: {var:v\u{0061}r}}) { })", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name. (Parse Error 1 line)");

testSyntaxError("(function a({ 0: {value:var}}) { })", "SyntaxError: Cannot use the keyword 'var' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a({ 0: {value:v\u0061r}}) { })", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a({ 0: {value:v\u{0061}r}}) { })", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name. (Parse Error 1 line)")

testSyntaxError("(function a({ 0: [var]}) { })", "SyntaxError: Cannot use the keyword 'var' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a({ 0: [v\u0061r]}) { })", "SyntaxError: Cannot use the keyword 'v\u0061r' as a variable name. (Parse Error 1 line)");
testSyntaxError("(function a({ 0: [v\u{0061}r]}) { })", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a variable name. (Parse Error 1 line)");

testSyntaxError("try { } catch(var) { }", "SyntaxError: Cannot use the keyword 'var' as a catch variable name. (Parse Error 1 line)");
testSyntaxError("try { } catch(v\u0061r) { }", "SyntaxError: Cannot use the keyword 'v\u0061r' as a catch variable name. (Parse Error 1 line)");
testSyntaxError("try { } catch(v\u{0061}r) { }", "SyntaxError: Cannot use the keyword 'v\u{0061}r' as a catch variable name. (Parse Error 1 line)");

testSyntaxError("class var { }", "SyntaxError: ES2015 class feature is not supported (Parse Error 1 line)");
testSyntaxError("class v\u0061r { }", "SyntaxError: ES2015 class feature is not supported (Parse Error 1 line)"); 
testSyntaxError("class v\u{0061}r { }", "SyntaxError: ES2015 class feature is not supported (Parse Error 1 line)"); 

// Allowed in non-keyword aware context.
testSyntax("({ v\u0061r: 'Cocoa' })");
testSyntax("({ v\u{0061}r: 'Cocoa' })");
