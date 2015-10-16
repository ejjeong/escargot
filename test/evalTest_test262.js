function testcase() {
    var x = -1;
    return function inner() {
        eval("var x = 1");
        print(x);
        if (x === 1)
            return true;
        return 1;
    } (); 
} 
print(testcase());
