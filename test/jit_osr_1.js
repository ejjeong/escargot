function foo(a, i) {
    var b = i + 3;
    var ret = a[i] + b;
    return ret;
}

var arr = [0, 5, 1.1];

print(foo(arr, 1));
print(foo(arr, 2));
