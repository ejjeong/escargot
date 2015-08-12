var o = {};
o.abc = 10;
o["ddd"] = "abc";
print(o.hasOwnProperty("ddd"));
print(o.hasOwnProperty("abc"));
print(o.hasOwnProperty("abcd"));
print(Object.prototype.hasOwnProperty);
print(Object.prototype.hasOwnProperty.apply(o, ["ddd"]));

var arr = [2, 3, 4, 5, 1, 6];
print(arr.hasOwnProperty);
print(arr.hasOwnProperty("1"));
print(Object.prototype.hasOwnProperty.apply(arr, ["1"]));
