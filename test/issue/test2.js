// FIXME continue doesn't work correctly in for-in statement when object has number-type index

var arr = [100,200,300];
for (var idx in arr) {
    print("idx " + idx);
    if (idx == 2)
        continue;
}

