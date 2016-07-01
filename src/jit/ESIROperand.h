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

#ifndef ESIROperand_h
#define ESIROperand_h

#ifdef ENABLE_ESJIT

#include "ESIRType.h"

namespace escargot {
namespace ESJIT {

class ESIROperand {
public:
    ESIROperand()
        : m_stackPos(0)
        , m_followingPopCount(0)
        , m_used(false)
    {
    }
    void setType(Type& type) { m_type = type; }
    void mergeType(Type& type) { m_type.mergeType(type); }
    Type getType() { return m_type; }
    void setStackPos(unsigned stackPos) { m_stackPos = stackPos; }
    unsigned getStackPos() { return m_stackPos; }
    void increaseFollowingPopCount() { m_followingPopCount++; }
    unsigned getFollowingPopCount() { return m_followingPopCount; }
    void setUsed() { m_used = true; }
    bool used() { return m_used; }

#ifndef NDEBUG
    friend std::ostream& operator<< (std::ostream& os, const ESIROperand& op);
    void dump(std::ostream& out, size_t index)
    {
        out << "tmp" << index << " ";
        m_type.dump(out);
        out << " (used:" << m_used << ")";
    }
#endif

private:
    Type m_type;
    unsigned m_stackPos;
    unsigned m_followingPopCount;
    bool m_used;
};

}}
#endif
#endif
