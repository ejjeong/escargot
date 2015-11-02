#ifndef ESJITMiddleend_h
#define ESJITMiddleend_h

#ifdef ENABLE_ESJIT

namespace escargot {

namespace ESJIT {

class ESGraph;

class ESGraphOptimization {
};

class ESGraphSSAConversion : ESGraphOptimization {
    public:
    static bool run(ESGraph* graph);
};

class ESGraphTypeInference : ESGraphOptimization {
    public:
    static bool run(ESGraph* graph);
};

class ESGraphSimplification : ESGraphOptimization {
    public:
    static bool run(ESGraph* graph);
};

class ESGraphLoadElimiation : ESGraphOptimization {
    public:
    static bool run(ESGraph* graph);
};

class ESGraphTypeCheckHoisting : ESGraphOptimization {
    public:
    static bool run(ESGraph* graph);
};

class ESGraphLoopInvariantCodeMotion : ESGraphOptimization {
    public:
    static bool run(ESGraph* graph);
};

class ESGraphDeadCodeEliminiation : ESGraphOptimization {
    public:
    static bool run(ESGraph* graph);
};

class ESGraphCommonSubexpressionElimination : ESGraphOptimization {
    public:
    static bool run(ESGraph* graph);
};

class ESGraphGlobalValueNumbering : ESGraphOptimization {
    public:
    static bool run(ESGraph* graph);
};

class ESGraphTypeModifier : ESGraphOptimization {
    public:
    static bool run(ESGraph* graph);
};

bool optimizeIR(ESGraph* graph);

}}
#endif
#endif



