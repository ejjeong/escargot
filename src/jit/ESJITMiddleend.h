/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

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
