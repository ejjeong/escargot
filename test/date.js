print("=== now ===");
print(Date.now());

//var date = new Date();
//var a = Date();
//print(a);
var date = new Date("2/3/2007 4:56:18");
//var date = new Date("March 21 2001 11:22:33");
//var date = new Date("January 1 2001 00:00:00 +0000");


print("=== getDate ===");
print(date.getDate());

print("=== getDay ===");
print(date.getDay());

print("=== getFullYear ===");
print(date.getFullYear());

print("=== getHours ===");
print(date.getHours());

print("=== getMinutes ===");
print(date.getMinutes());

print("=== getMonth ===");
print(date.getMonth());

print("=== getSeconds ===");
print(date.getSeconds());

print("=== getTime ===");
var t = date.getTime();
print(t);

print("=== getTimezoneOffset ===");
print(date.getTimezoneOffset());

var date1 = new Date();
date1.setTime(t);

print("=== getDate ===");
print(date1.getDate());

print("=== getDay ===");
print(date1.getDay());

print("=== getFullYear ===");
print(date1.getFullYear());

print("=== getHours ===");
print(date1.getHours());

print("=== getMinutes ===");
print(date1.getMinutes());

print("=== getMonth ===");
print(date1.getMonth());

print("=== getSeconds ===");
print(date1.getSeconds());

print("=== getTime ===");
print(date1.getTime());

print("=== getTimezoneOffset ===");
print(date1.getTimezoneOffset());

