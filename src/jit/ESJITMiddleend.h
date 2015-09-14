#ifndef ESJITMiddleend_h
#define ESJITMiddleend_h

namespace escargot {

namespace ESJIT {

class ESGraph;

class IRGraphOptimization {
};

class IRGraphSimplification : IRGraphOptimization{
public:
    static void run(ESGraph* graph);
};

class IRGraphLoadElimiation : IRGraphOptimization {
public:
    static void run(ESGraph* graph);
};

class IRGraphTypeCheckHoisting : IRGraphOptimization {
public:
    static void run(ESGraph* graph);
};

class IRGraphLoopInvariantCodeMotion : IRGraphOptimization {
public:
    static void run(ESGraph* graph);
};

class IRGraphDeadCodeEliminiation : IRGraphOptimization {
public:
    static void run(ESGraph* graph);
};

class IRGraphCommonSubexpressionElimination : IRGraphOptimization {
public:
    static void run(ESGraph* graph);
};

class IRGraphGlobalValueNumbering : IRGraphOptimization {
public:
    static void run(ESGraph* graph);
};

void optimizeIR(ESGraph* graph);

}}
#endif
