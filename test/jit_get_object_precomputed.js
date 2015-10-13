function foo(a) {
	var ret = obj.bb;
	return ret;
}

var obj = {aa: 1, bb :2};

print(foo(obj));
print(foo(obj));
print(foo(obj));
