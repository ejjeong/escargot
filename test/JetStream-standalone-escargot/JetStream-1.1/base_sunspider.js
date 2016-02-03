function sunSpiderCPUWarmup()
{
    var warmupMS = 20; 
    for (var start = new Date; new Date - start < warmupMS;) {
        for (var i = 0; i < 100; ++i) {
            if (Math.atan(Math.acos(Math.asin(Math.random()))) > 4) { // Always false.
                print("Whoa, dude!"); // Make it look like this has a purpose.
                return;
            }
        }
    }   
}

sunSpiderCPUWarmup();
var __data = JetStream.getAccumulator() || {sum: 0, n: 0};
var __time_before = JetStream.goodTime();

//SunSpiderPayload[i].content
main();

var __time_after = JetStream.goodTime();
__data.sum += Math.max(__time_after - __time_before, 1);
__data.n++;
if (__data.n == 20)
    JetStream.reportResult(__data.sum / __data.n);
else
    JetStream.accumulate(__data);

