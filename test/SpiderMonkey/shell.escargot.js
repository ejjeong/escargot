/* add version() to test js1_2 by youri */
function version() {
  return 120;
}

/* add finalizeCount() & makeFinalizeObserver by youri */
/* it doesn't needed probably*/
function finalizeCount() {
  return 0;
}

function makeFinalizeObserver() {
  return 0;
}

var origLoad = load;
load = function(path) {
    origLoad("test/SpiderMonkey/" + path);
}

function evalcx(str, object) {
    if (object != undefined)
        throw "evalcx() not supported";
    if (str == "lazy" && object == undefined)
        throw "evalcx() not supported";
    if (str == '')
        throw "evalcx() not supported";

    eval(str);
}

function uneval(value) {
    return "" + value;
}

