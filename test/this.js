print(this);
var o = {};
o.asdf = function() { print(this) };
o.asdf();
var asdf = o.asdf;
asdf();
