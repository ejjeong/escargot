var WScript = {
    Echo : print,
    LoadScriptFile : function(path) {
        try {
            load(path)
        } catch (e) {
            load("test\\chakracore\\UnitTestFramework\\" + path);
        }
    }
};

function CollectGarbage() {
    gc();
}
