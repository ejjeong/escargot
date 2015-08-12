print(Boolean);
Boolean.prototype.a = 10;
print(Boolean.prototype.toString());
print(Boolean.prototype.valueOf());
print(Boolean.prototype.a);

var b = Boolean(true);
print(b.toString());
var bb = new Boolean(false);
print(bb.valueOf());
