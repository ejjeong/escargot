if(typeof dbgBreak == "undefined")
	dbgBreak = function(){ print("dbgBreak"); }

function hook() {
print.apply(this,arguments)
}

this.nativeFlushQueueImmediate = function(a) {
print("this.nativeFlushQueueImmediate");
print(JSON.stringify(a));
print("this.nativeFlushQueueImmediate--------------");
return ;
}

console = {}; console.log = hook; console.error = hook; console.info = hook;
                Object.prototype.__defineGetter__ = function(prop, fn) {
                  Object.defineProperty(this, prop, {
                        get:fn,
                        configurable:true,
                        enumerable:true
                });
                }
                
                Object.prototype.__defineSetter__ = function(prop, fn) {
                  Object.defineProperty(this, prop, {
                        set:fn,
                        configurable:true,
                        enumerable:true
                });
                }
