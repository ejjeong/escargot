var CDjsFiles = [
    "constants.js",
    "util.js",
    "red_black_tree.js",
    "call_sign.js",
    "vector_2d.js",
    "vector_3d.js",
    "motion.js",
    "reduce_collision_set.js",
    "simulator.js",
    "collision.js",
    "collision_detector.js",
    "benchmark.js"
];

var code = "";
//for (var i = 0; i < CDjsFiles.length; ++i)
//    code += "<script src=\"cdjs/" + CDjsFiles[i] + "\"></script>\n";
code += "<script>\n";
code += "print(\"running...\");\n";
code += "var __result;// = benchmark();\n";
code += "print(\"got result: \" + __result);\n";
code += "JetStream.reportResult(__result);\n";
code += "</script>";

print("code = " + code);

JetStream.addPlan({
    name: "cdjs",
    benchmarks: [{
        name: "cdjs",
        category: "Latency",
        unit: "ms"
    }],
    code: code
});

