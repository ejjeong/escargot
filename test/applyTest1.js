var o = {};
o.abc = 10;
o["ddd"] = "abc";
print(o.hasOwnProperty("ddd"));
print(o.hasOwnProperty("abc"));
print(o.hasOwnProperty("abcd"));
print(Object.prototype.hasOwnProperty);
print(Object.prototype.hasOwnProperty.apply(o, ["ddd"]));
