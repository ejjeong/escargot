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

#include "ESGraph.h"
#include "ESIR.h"

namespace escargot {
namespace ESJIT {

bool ESGraphTypeModifier::run(ESGraph* graph)
{
    unsigned doubleCnt = 0;
    unsigned intCnt = 0;
    for (size_t i = 0; i < graph->operandsSize(); i++) {
        Type tp = graph->getOperandType(i);
        if (tp.isDoubleType()) {
            doubleCnt++;
        } else if (tp.isInt32Type()) {
            intCnt++;
        }
    }

    // printf("%f\n", (float)doubleCnt / (float)(intCnt + doubleCnt));
    // if ((float)doubleCnt / (float)(intCnt + doubleCnt) > 0.1f) {
    // if (doubleCnt) {
        if (0) {
            for (size_t i = 0; i < graph->operandsSize(); i++) {
                Type tp = graph->getOperandType(i);
                if (tp.isInt32Type()) {
                    graph->setOperandType(i, Type(TypeDouble));
                }
            }
        }
        return true;
    }

}}
#endif
