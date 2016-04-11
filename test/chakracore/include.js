var WScript = {
    Echo : print,
    LoadScriptFile : load,
};

function CollectGarbage() {
    gc();
}
