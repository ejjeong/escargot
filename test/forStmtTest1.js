var i;

print("Case 1");
for (i = 0; i < 10; i++)
    print(i);

print("Case 2");
for (i = 0; i < 10;) {
    print(i);
	i++;
}

print("Case 3");
i = 0;
for (;;) {
	print(i);
	i++;
	if (i == 10)
		break;
}
