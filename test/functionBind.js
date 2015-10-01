function list() {
  return Array.prototype.slice.call(arguments);
}

print(list1 = list(1, 2, 3)); // [1, 2, 3]

// Create a function with a preset leading argument
var leadingThirtysevenList = list.bind(undefined, 37);

print(list2 = leadingThirtysevenList()); // [37]
print(list3 = leadingThirtysevenList(1, 2, 3)); // [37, 1, 2, 3]
