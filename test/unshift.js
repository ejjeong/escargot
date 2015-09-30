var arr = [1, 2, 3];
print("length = " + arr.length);

print("= shift =");
arr.shift();
print("length = " + arr.length);
for (var i = 0; i < arr.length; i++)
	print(arr[i]);

print("= unshift =");
arr.unshift({l: 11});
print("length = " + arr.length);
for (var i = 0; i < arr.length; i++)
	print(arr[i]);


