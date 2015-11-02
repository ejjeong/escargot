#ifndef ESJITFrontend_h
#define ESJITFrontend_h

#ifdef ENABLE_ESJIT

namespace escargot {

class CodeBlock;

namespace ESJIT {

class ESGraph;

ESGraph* generateIRFromByteCode(CodeBlock* codeBlock);

}}
#endif
#endif


