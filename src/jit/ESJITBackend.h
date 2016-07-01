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

#ifndef ESJITBackend_h
#define ESJITBackend_h

#ifdef ENABLE_ESJIT

#include "ESIRType.h"
#include "nanojit.h"

namespace escargot {

class ESVMInstance;

namespace ESJIT {

class ESGraph;
class ESIR;

class NativeGenerator {
public:
    NativeGenerator(ESGraph* graph);
    ~NativeGenerator();

    bool nanojitCodegen(ESVMInstance* instance);
    bool nativeCodegen();

    JITFunction nativeCode()
    {
        ASSERT(m_succeeded);
        return reinterpret_cast<JITFunction>(m_f->code());
    }
    void setSucceeded() { m_succeeded = true; }

private:
    nanojit::LIns* nanojitCodegen(ESIR* ir);

    void setTmpMapping(size_t irIndex, nanojit::LIns* ins)
    {
        // printf("tmpMap[%lu] = %p\n", irIndex, ins);
        ASSERT(irIndex < m_tmpToLInsMapping.size());
        ASSERT(ins);
        m_tmpToLInsMapping[irIndex] = ins;
    }

    nanojit::LIns* getTmpMapping(size_t irIndex)
    {
        // printf("= tmpMap[%lu]\n", irIndex);
        ASSERT(irIndex < m_tmpToLInsMapping.size());
        ASSERT(m_tmpToLInsMapping[irIndex]);
        return m_tmpToLInsMapping[irIndex];
    }
    nanojit::LIns* generateOSRExit(size_t currentByteCodeIndex, nanojit::LIns* in = nullptr);
    nanojit::LIns* generateTypeCheck(nanojit::LIns* in, Type type, size_t currentByteCodeIndex);
    nanojit::LIns* boxESValue(nanojit::LIns* value, Type type);
    nanojit::LIns* unboxESValue(nanojit::LIns* value, Type type);
#ifndef ESCARGOT_64
    nanojit::LIns* boxESValueFromTagAndPayload(nanojit::LIns* tag, nanojit::LIns* payload);
    nanojit::LIns* getTagFromESValue(nanojit::LIns* boxedValue);
    nanojit::LIns* getPayloadFromESValue(nanojit::LIns* boxedValue);
#endif

    nanojit::LIns* toDoubleDynamic(nanojit::LIns* in, Type type);
    nanojit::LIns* toInt32Dynamic(nanojit::LIns* in, Type type); // always truncates

    nanojit::LIns* getOffsetAsPointer(nanojit::LIns* in);

    ESGraph* m_graph;
    std::vector<nanojit::LIns*, CustomAllocator<nanojit::LIns*> > m_tmpToLInsMapping;

    nanojit::LogControl m_lc;
    nanojit::Config m_config;
    nanojit::Allocator* m_alloc;
    nanojit::Assembler* m_assm;
    nanojit::LirBuffer* m_buf;
    nanojit::Fragment* m_f;
    nanojit::LirWriter* m_out;
    nanojit::SideExit* m_exit;
    nanojit::GuardRecord* m_rec;

    nanojit::LIns* m_zeroP;
    nanojit::LIns* m_oneI;
    nanojit::LIns* m_zeroI;
    nanojit::LIns* m_true;
    nanojit::LIns* m_false;

#ifdef ESCARGOT_64
    nanojit::LIns* m_tagMaskQ;
    nanojit::LIns* m_intTagQ;
    nanojit::LIns* m_intTagComplementQ;
    nanojit::LIns* m_booleanTagQ;
    nanojit::LIns* m_booleanTagComplementQ;
    nanojit::LIns* m_nullQ;
    nanojit::LIns* m_undefinedQ;
    nanojit::LIns* m_emptyQ;
    nanojit::LIns* m_emptyD;
    nanojit::LIns* m_doubleEncodeOffsetQ;
    nanojit::LIns* m_zeroQ;
    nanojit::LIns* undefined() { return m_undefinedQ; }
    nanojit::LIns* empty() { return m_emptyQ; }
#else
    nanojit::LIns* m_int32TagI;
    nanojit::LIns* m_booleanTagI;
    nanojit::LIns* m_nullTagI;
    nanojit::LIns* m_undefinedTagI;
    nanojit::LIns* m_pointerTagI;
    nanojit::LIns* m_emptyValueTagI;
    nanojit::LIns* m_deletedValueTagI;
    nanojit::LIns* m_lowestTagI;
    nanojit::LIns* undefined() { return m_out->insImmD(bitwise_cast<double>(ESValue().asRawData())); }
    nanojit::LIns* empty() { return m_out->insImmD(bitwise_cast<double>(ESValue(ESValue::ESEmptyValueTag::ESEmptyValue).asRawData())); }
#endif

    nanojit::LIns* m_thisValueP;
    nanojit::LIns* m_instanceP;
    nanojit::LIns* m_contextP;
    nanojit::LIns* m_globalObjectP;
    nanojit::LIns* m_cachedDeclarativeEnvironmentRecordESValueP;

    bool m_succeeded;
};

JITFunction generateNativeFromIR(ESGraph* graph, ESVMInstance* instance);

JITFunction addDouble();
int nanoJITTest();

}}
#endif
#endif
