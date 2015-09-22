var a = {aa: 1, bb: 2};
print(a.aa);
print(a.bb);
delete a.aa;
print(a.aa);
print(a.bb);

print(a.bb);
delete a;
print(a.bb);
