function f(x) {
  return (x < 10)? (x < 5)? 3 : 7 : -5;
}
/*function f(x) {
  return (x < 10)? 7 : -5;
}*/
print(f(11));
print(f(5));
print(f(7));
print(f(3));
print(f(20));
