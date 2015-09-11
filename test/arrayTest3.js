var obj = {};
print("-----push-----");
obj.push = Array.prototype.push;
obj.push(4);
print(obj);
print(obj[0]);
print(obj["0"]);

print("-----concat-----");
obj.concat = Array.prototype.concat;
obj.push("abc");
print(obj.length);
print(obj.concat(["aaa", 3, 2]).toString());
print(obj.concat(["a", 3, 2]).length);

print("-----indexOf-----");
obj.indexOf = Array.prototype.indexOf;
print(obj.indexOf("abc"));
print(obj.indexOf(4));

print("-----join-----");
obj.join = Array.prototype.join;
print(obj.join("|"));
print(obj.join(''));

print("-----map-----");
obj[0] = 4;
obj[1] = 9;
obj[2] = 16;
obj.map = Array.prototype.map;
var mapped = obj.map(Math.sqrt);
print(mapped.toString());

print("-----pop-----");
obj.pop = Array.prototype.pop;
print(obj);
print(obj.length);
print(obj.pop());
print(obj.pop());
print(obj);
print("-----slice-----");
obj.slice = Array.prototype.slice;
obj.push(1, 2, "abc", 5, 5, 7);
print(obj.slice(2, 5).toString());
print("-----sort-----");
//TODO
obj.sort = Array.prototype.sort;

print("-----splice-----");
obj.splice = Array.prototype.splice;
print("(1) " + obj.splice());
print("(2) " + obj.length);
print("(3) " + obj.splice(5));
print("(4) " + obj.length);
print("(5) " + obj.splice(3, 1, "abc", "def"));
print("(6) " + obj.length);
print("(7) " + obj.splice(3, 4, "abc", "def"));
obj.abc = 4;
print("(8) " + obj.splice());
print("(9) " + obj.length);
print("(10) " +obj.splice(3));
print("(11) " + obj.length);
print("(12) " + obj.splice(2, 1));
print("(13) " + obj.length);
print("(14) " + obj.splice(3, 4, "ab", "def"));
print("(15) " + obj.length);

print("-----toString-----");
obj.toString = Array.prototype.toString;
print(obj.toString());

//TODO
var arr = [3, 4, 2, "aaaaa"];
arr.join = function() {
    return arr[0];
}
print(arr.toString());


