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
#include "ESIR.h"

#include "ESGraph.h"

namespace escargot {
namespace ESJIT {

const char* ESIR::getOpcodeName()
{
    switch (m_opcode) {
#define RETURN_OPCODE_NAME(name, unused) case ESIR::name: return #name;
        FOR_EACH_ESIR_OP(RETURN_OPCODE_NAME)
#undef  RETURN_OPCODE_NAME
    default: RELEASE_ASSERT_NOT_REACHED();
    }
}

uint32_t ESIR::getFlags()
{
    auto getFlag = [] (uint32_t flag = 0) -> uint32_t {
        return flag;
    };
    switch (m_opcode) {
#define RETURN_ESIR_FLAG(name, flag) case ESIR::name: return getFlag(flag);
        FOR_EACH_ESIR_OP(RETURN_ESIR_FLAG)
#undef  RETURN_ESIR_FLAG
    default: RELEASE_ASSERT_NOT_REACHED();
    }
}

bool ESIR::returnsESValue()
{
    return getFlags() & 1;
}

#ifndef NDEBUG
void ESIR::dump(std::ostream& out)
{
    out << getOpcodeName();
}

void BranchIR::dump(std::ostream& out)
{
    out << "tmp" << m_targetIndex << ": ";
    ESIR::dump(out);
    out << " if tmp" << m_operandIndex;
    out << ", goto B" << m_trueBlock->index() << ", else goto B" << m_falseBlock->index();
}

void JumpIR::dump(std::ostream& out)
{
    out << "tmp" << m_targetIndex << ": ";
    ESIR::dump(out);
    out << " goto B" << m_targetBlock->index();
}
#endif

}}
#endif
