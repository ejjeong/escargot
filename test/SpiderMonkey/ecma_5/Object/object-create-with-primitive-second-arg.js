function Symbol(){};

[1, "", true, Symbol(), undefined].forEach(function(props) {
    assertEq(Object.getPrototypeOf(Object.create(null, props)), null);
});

assertThrowsInstanceOf(function(){ Object.create(null, null) }, TypeError);

if (typeof reportCompare === "function")
    reportCompare(true, true);
