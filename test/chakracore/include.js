var nativePrint = print;
var WScript = {
    Echo : function() {
        var length = arguments.length;
        var finalResult = "";
        for (var i = 0; i < length; i++) {
            if (i != 0)
                finalResult += " ";
            var arg = arguments[i];
            if (typeof arg == undefined || arg == null)
                finalResult += arg;
            else
                finalResult += (arg.toString());
        }
        nativePrint(finalResult);
    },
    LoadScriptFile : function(path) {
        try {
            load(path)
        } catch (e) {
            load("test\\chakracore\\UnitTestFramework\\" + path);
        }
    },
    Arguments : ["summary"]
};

function CollectGarbage() {
    gc();
}
