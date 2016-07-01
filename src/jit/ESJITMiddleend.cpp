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

#ifdef ENABLE_ESJIT

#include "Escargot.h"
#include "ESJITMiddleend.h"

#include "ESIR.h"
#include "ESJIT.h"

namespace escargot {
namespace ESJIT {

bool optimizeIR(ESGraph* graph)
{
    if (!ESGraphSSAConversion::run(graph)) {
        LOG_VJ("Failed to run SSAConversion\n");
        return false;
    }

    if (!ESGraphTypeModifier::run(graph)) {
        LOG_VJ("Failed to run GraphTypeModifier\n");
        return false;
    }

    if (!ESGraphTypeInference::run(graph)) {
        LOG_VJ("Failed to run TypeInference\n");
        return false;
    }
#if 0
    ESGraphSimplification::run(graph);
    ESGraphLoadElimiation::run(graph);
    ESGraphTypeCheckHoisting::run(graph);
    ESGraphLoopInvariantCodeMotion::run(graph);
    ESGraphDeadCodeEliminiation::run(graph);
    ESGraphCommonSubexpressionElimination::run(graph);
    ESGraphGlobalValueNumbering::run(graph);
#endif

    return true;
}

}}
#endif
