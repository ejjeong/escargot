var JetStream = (function() {
    var isRunning;
    var hasAlreadyRun;
    var currentPlan;
    var currentIteration;
    var numberOfIterations;
    var accumulator;
    var selectBenchmark;

    var givenPlans = [];
    var givenReferences = {};
    var categoryNames;
    var plans;
    var benchmarks;

    var filename = {
        "3d-cube" : "sunspider/3d-cube.js",
        "3d-raytrace" : "sunspider/3d-raytrace.js",
        "base64" : "sunspider/base64.js",
        "crypto-aes" : "sunspider/crypto-aes.js",
        "crypto-md5" : "sunspider/crypto-md5.js",
        "crypto-sha1" : "sunspider/crypto-sha1.js",
        "date-format-tofte" : "sunspider/date-format-tofte.js",
        "date-format-xparb" : "sunspider/date-format-xparb.js",
        "n-body" : "sunspider/n-body.js",
        "regex-dna" : "sunspider/regex-dna.js",
        "tagcloud" : "sunspider/tagcloud.js",
    }
    function addPlan(plan)
    {
       givenPlans.push(plan); 
    }
    function addReferences(references)
    {
        for (var s in references)
            givenReferences[s] = references[s];
    }

    function reset()
    {
        var categoryMap = {}; 
        benchmarks = []; 
  
        for (var i = 0; i < givenPlans.length; ++i) {
            var plan = givenPlans[i];
            for (var j = 0; j < plan.benchmarks.length; ++j) {
                var benchmark = plan.benchmarks[j];
                benchmarks.push(benchmark);
                benchmark.plan = plan;
                benchmark.reference = givenReferences[benchmark.name] || 1;
            }
        }
        // XXX # of plans : 37, # of benchmarks : 39

        plans = givenPlans;

        for (var i = benchmarks.length; i--;) {
            benchmarks[i].results = [];
            benchmarks[i].times = [];
        }

        currentIteration = 0;
        currentPlan = -1;
        isRunning = false;
        hasAlreadyRun = false;
    }

    function runCode(plan)
    {
        var benchmarkSuiteType = plan.ancestor; // sunspider, octane, cdjs, ...

        if(benchmarkSuiteType == "sunspider") {
            sunSpiderCPUWarmup();
            var __data = JetStream.getAccumulator() || {sum: 0, n: 0};
            var __time_before = JetStream.goodTime();

            load(filename[plan.name]);

            var __time_after = JetStream.goodTime();
//            print(__time_after - __time_before);
            __data.sum += Math.max(__time_after - __time_before, 1);
            __data.n++;
            if (__data.n == 20){
//                gc(); // v8 has no gc() function 
                JetStream.reportResult(__data.sum / __data.n);
            }
            else
                JetStream.accumulate(__data);
        
        }/* else if {
        }*/

    }

    function computeRawResults()
    {
        function rawResultsLine(values) {
            return {
                result: values,
                statistics: null//computeStatistics(values)
            };
        }

        var rawResults = {};
        for (var i = 0; i < benchmarks.length; ++i) {
            var line = rawResultsLine(benchmarks[i].results);
            line.times = benchmarks[i].times;
            line.category = benchmarks[i].category;
            line.reference = benchmarks[i].reference;
            rawResults[benchmarks[i].name] = line;
            print(benchmarks[i].name);
            print(line.result);
        }
      //  rawResults.geomean = rawResultsLine(computeGeomeans(allSelector));

        print(rawResults);
        return rawResults;
    }

    function end()
    {
        print("Raw results:", JSON.stringify(computeRawResults()));

        isRunning = false;
        hasAlreadyRun = true;
    }

    function iterate()
    {
        ++currentPlan;
        //updateGeomeans();

        if (currentPlan >= plans.length) {
            if (++currentIteration >= numberOfIterations) {
                end();
                return;
            } else {
                currentPlan = 0;
            }
        }
        accumulator = void 0;
        
        if(!isRunning)
            return;
        var d = new Date();
        while(new Date() - d < 100) {} // sleep for 100ms
        runCode(plans[currentPlan]);
    }
    function start()
    {
        reset();
        isRunning = true;
        iterate();
    }

    function initialize()
    {
        reset();
        numberOfIterations = 1;
//        numberOfIterations = 3;   // to iterate more than 1 time, regexp - tagcloud problem should be fixed
    }

    function reportResult()
    {   
        var plan = plans[currentPlan];
        for (var i = plan.benchmarks.length; i--;) {
            var benchmark = plan.benchmarks[i];
            benchmark.times.push(arguments[i]);
            benchmark.results.push(100 * benchmark.reference / arguments[i]);
//            displayResultMessage(
//                benchmark.name,
//                formatResult(benchmark.results, plan.benchmarks[i]),
//                "result");
        }
        iterate();
    }   

    function accumulate(data)
    {   
        accumulator = data;
        
        if (!isRunning)
            return;
        runCode(plans[currentPlan]);
    }   

    function getAccumulator()
    {   
        return accumulator;
    }   

    return {
        addPlan: addPlan,
        addReferences: addReferences,
        initialize: initialize,
        switchToQuick: function() { switchMode("quick") },
        switchToNormal: function() { switchMode("normal") },
        switchToLong: function() { switchMode("long") },
        start: start,
        reportResult: reportResult,
//        reportError: reportError,
        accumulate: accumulate,
        getAccumulator: getAccumulator,
        goodTime: Date.now
    }; 
})();

// It is converted form of 'index.html'


load("SunSpiderPayload.js");
load("SimplePayload.js");
//load("JetStreamDriver.js");
load("SunSpiderSetup.js");
//load("SimpleSetup.js");
//load("OctaneSetup.js");
//load("Octane2Setup.js");
//load("CDjsSetup.js");
load("Reference.js");

JetStream.initialize();
JetStream.start();



