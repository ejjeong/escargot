function foo(b) {
	var m = b;
	m <<= 1;	
	return m;
}

print(foo(1));
print(foo(2));
print(foo(5));

