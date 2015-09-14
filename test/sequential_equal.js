//var a = {};
var a = {a1: 3, a2: 4, a3: 7};
var c = function() {print(11);};

print(a.aa);
print(a.bb);
print(c);

a.aa = a.bb = c;

print(a.aa);
print(a.bb);
print(c);
