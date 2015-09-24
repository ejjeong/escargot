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

var aa = new Int32Array(2);
for (var i = 0; i < aa.length; i++)
    aa[i] = 100;
print("== set ==");
a.set(aa, 2);
print(a);
print("length = " + a.length);
for (var i = 0; i < a.length; i++) {
	print(a[i]);
}
var f = new Float32Array(2);
for (var i = 0; i < f.length; i++)
    f[i] = 3.11;
a.set(f, 1);
print(a);
print("length = " + a.length);
for (var i = 0; i < a.length; i++) {
	print(a[i]);
}
