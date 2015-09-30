var a = "/bin/this.program";
var b = unescape(encodeURIComponent(a));
print(typeof b);
print(b);

var c = "$©";
var d = unescape(encodeURIComponent(c));
print(typeof d);
print(d);
