function foo(a) {
    var res = a && 20;
    var res2 = a || 10;
    return res + res2;
}
print(foo(10));
print(foo(0));
print(foo(10));
print(foo(10));
print(foo(10));
print(foo(0));
print(foo(0));
