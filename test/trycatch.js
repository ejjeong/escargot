function f() {
    try {
        throw '';
    } catch(e) {
        var a = 10;
    }
    print(a);
}
f();
