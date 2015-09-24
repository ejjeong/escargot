var o = {a: 7, get b() {return this.a + 1;}, set b(x) {this.a = x / 2}};
print(o.a);
print(o.b);
o.b = 10;
print(o.a);
print(o.b);

// Create a user-defined object.
var obj = {};

// Add an accessor property to the object.
Object.defineProperty(obj, "newAccessorProperty", {
    set: function (x) {
        print("in property set accessor");
        this.newaccpropvalue = x;
    },
    get: function () {
        print("in property get accessor");
        return this.newaccpropvalue;
    },
    enumerable: true,
    configurable: true,
});

// Set the property value.
obj.newAccessorProperty = 30;
print(obj.newaccpropvalue);
print(obj.newAccessorProperty);
//print(obj);
