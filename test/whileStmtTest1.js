var test=1;
while (test < 0x100) {
    print(test);
    test <<= 1;
}
print(test);
