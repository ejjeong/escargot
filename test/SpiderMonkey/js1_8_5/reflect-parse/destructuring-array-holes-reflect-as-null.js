// escargot-skip: Reflect not supported
// |reftest| skip-if(!xulRuntime.shell)
function test() {

// Bug 632027: array holes should reflect as null
assertExpr("[,]=[,]", aExpr("=", arrPatt([null]), arrExpr([null])));

}

runtest(test);
