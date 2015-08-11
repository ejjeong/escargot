var str = "Hello Wolld";
var matched = str.match(/.ll./g);
print(matched.length);
for (var i = 0; i < matched.length; i++) {
	print(matched[i]);
}

var replaced = str.replace(/.ll./g, "abc");
print("r1");
print(replaced);

var replaced = str.replace("o", "abc");
print("r1-1");
print(replaced);

// regexp-dna
replaced = str.replace(/agggtaaa|tttaccct/ig, "abc");
print("r2");
print(replaced);

replaced = str.replace(/[cgt]gggtaaa|tttaccc[acg]/ig, "abc");
print("r3");
print(replaced);

replaced = str.replace(/a[act]ggtaaa|tttacc[agt]t/ig, "abc");
print("r4");
print(replaced);

replaced = str.replace(/ag[act]gtaaa|tttac[agt]ct/ig, "abc");
print("r5");
print(replaced);

replaced = str.replace(/agg[act]taaa|ttta[agt]cct/ig, "abc");
print("r6");
print(replaced);

replaced = str.replace(/aggg[acg]aaa|ttt[cgt]ccct/ig, "abc");
print("r7");
print(replaced);

replaced = str.replace(/agggt[cgt]aa|tt[acg]accct/ig, "abc");
print("r8");
print(replaced);

replaced = str.replace(/agggta[cgt]a|t[acg]taccct/ig, "abc");
print("r9");
print(replaced);

replaced = str.replace(/agggtaa[cgt]|[acg]ttaccct/ig, "abc");
print("r10");
print(replaced);

replaced = str.replace(/>.*\n|\n/g, "abc");
print("r11");
print(replaced);

// crypto-aes
replaced = str.replace(/[\0\t\n\v\f\r\xa0'"!-]/g, "abc");
print("r12");
print(replaced);

replaced = str.replace(/!\d\d?\d?!/g, "abc");
print("r13");
print(replaced);


replaced = str.replace(/[\u0080-\u07ff]/g, "abc");
print("r14");
print(replaced);

replaced = str.replace(/[\u0800-\uffff]/g, "abc");
print("r15");
print(replaced);

replaced = str.replace(/[\u00c0-\u00df][\u0080-\u00bf]/g, "abc");
print("r16");
print(replaced);

replaced = str.replace(/[\u00e0-\u00ef][\u0080-\u00bf][\u0080-\u00bf]/g, "abc");
print("r17");
print(replaced);

// date-format-xparb
replaced = str.replace(/^.*? ([A-Z]{3}) [0-9]{4}.*$/, "abc");
print("r18");
print(replaced);

replaced = str.replace(/^.*?\(([A-Z])[a-z]+ ([A-Z])[a-z]+ ([A-Z])[a-z]+\)$/, "abc");
print("r19");
print(replaced);

replaced = str.replace(/('|\\)/g, "abc");
print("r20");
print(replaced);

// string-tagcloud
replaced = str.replace(/^[\],:{}\s]*$/, "abc");
print("r21");
print(replaced);

replaced = str.replace(/\\./g, "abc");
print("r22");
print(replaced);

replaced = str.replace(/"[^"\\\n\r]*"|true|false|null|-?\d+(?:\.\d*)?(:?[eE][+\-]?\d+)?/g, "abc");
print("r23");
print(replaced);

replaced = str.replace(/(?:^|:|,)(?:\s*\[)+/g, "abc");
print("r24");
print(replaced);

replaced = str.replace(/[\x00-\x1f\\"]/g, "abc");
print("r25");
print(replaced);

// string-unpack-code
replaced = str.replace(/^/, "abc");
print("r26");
print(replaced);

// string-validate-input
replaced = str.replace(/^[a-zA-Z0-9\-\._]+@[a-zA-Z0-9\-_]+(\.?[a-zA-Z0-9\-_]*)\.[a-zA-Z]{2,3}$/, "abc");
print("r27");
print(replaced);

