var arr = [1, 2, "abc"];
var c = 2;
print(arr);
print(arr[0]);
print(arr.indexOf("abc"));
print(arr[2]);
print(arr[c]);
arr[3] = 5;
arr[4] = 5;
print(arr.indexOf(5));
print(arr[3]-1);
print(arr.length);
arr.push(7);
print(arr.length);
print(arr[5]);
print(arr);
print(arr.join("|"));
print(arr.join(''));

print(arr.splice());
print(arr);
print(arr.splice(5));
print(arr);
print(arr.splice(3, 1, "abc", "def"));
print(arr);
print(arr.splice(3, 4, "abc", "def"));
print(arr);

print('--------------')

arr.abc = 4;
print(arr.splice());
print(arr);
print(arr.splice(3));
print(arr);
print(arr.splice(2, 1));
print(arr);
print(arr.splice(3, 4, "ab", "def"));
print(arr);

print(arr.slice(2, 3));
print(arr);
