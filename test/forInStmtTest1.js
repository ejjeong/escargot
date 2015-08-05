var arr = {
    a:1,
    b:2,
    c:3,
    d:4,
    e:5,
    f:6,
    g:7,
}
print(arr);

for (i in arr) {
    print(i + " " +  arr[i]);
    arr.h = 8;
    arr.i = 9;
}
