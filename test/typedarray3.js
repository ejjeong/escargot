print("== new TypedArray ==");
var a = new Int32Array(5);
for (var i = 0; i < a.length; i++) {
	a[i] = i;
}
print(a);
print("length = " + a.length);
for (var i = 0; i < a.length; i++) {
	print(a[i]);
}

print("== subarray ==");
var b = a.subarray(1, 3);
print(b);
print(b.toString());
print("length = " + b.length);
for (var i = 0; i < b.length; i++) {
	print(b[i]);
}
