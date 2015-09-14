#ifndef ESJITFrontend_h
#define ESJITFrontend_h

namespace escargot {

class CodeBlock;

namespace ESJIT {

class ESGraph;

ESGraph* generateIRFromByteCode(CodeBlock* codeBlock);

}}
#endif
