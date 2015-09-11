var obj = {};
obj.push = Array.prototype.push;
obj.push(4);
print(obj);
print(obj[0]);
print(obj["0"]);
