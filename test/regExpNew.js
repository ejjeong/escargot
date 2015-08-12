var reg = new RegExp('d');
var str = "defdef";
var ret = str.replace(reg, "abc");
print(ret);

reg = new RegExp('d', 'g');
ret = str.replace(reg, "abc");
print(ret);

