var tmp = {a:10, b:"abc", c:[1,2,3], d:undefined};
print(JSON.stringify(tmp));

print(JSON.stringify(NaN));
print(JSON.stringify('abc"d"ef'));
print(JSON.stringify([1, 3, 2]));

var state = {
    registerA: 1,
    registerB: 2,
    registerC: 3,
    registerE: 4,
    registerF: 5,
    registersHL: 6,
    programCounter: 7,
    stackPointer: 8,
    sumROM : 9,
    sumMemory: 10,
    sumMBCRam: 11,
    sumVRam: 12
}
print(JSON.stringify(state));
