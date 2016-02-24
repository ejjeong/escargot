
function edenGC() {
    gc();
}

function fullGC() {
    gc();
}

function DFGTrue() {
    return true;
}

function debug(msg) {
    print(msg);
}

function numberOfDFGCompiles() {}
function neverInlineFunction() {}

var self = this;

self.testRunner = {
    neverInlineFunction: neverInlineFunction,
    numberOfDFGCompiles: numberOfDFGCompiles
};

function noInline(theFunction)
{
    if (!self.testRunner)
        return;

    testRunner.neverInlineFunction(theFunction);
}

function isInt32(e) {
    return false; // no error
    // return true; // Uncaught Error: bad result: true
}


function loadWebAssembly(url) {
    load(url); // should load wsam file here
}


function OSRExit() {

}

function effectful42() {
    return 42;
}

/*

function hasCustomProperties(o){
    if(o) return true; // true : Uncaught Error : object should't have custom properties yet
    else return false; // false: Uncaught Error : object should have custom properties already
}

*/
