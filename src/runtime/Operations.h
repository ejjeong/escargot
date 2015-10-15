#ifndef Operations_h
#define Operations_h

#include "ESValue.h"

namespace escargot {

NEVER_INLINE ESValue plusOperation(const ESValue& left, const ESValue& right);
NEVER_INLINE ESValue minusOperation(const ESValue& left, const ESValue& right);
NEVER_INLINE ESValue modOperation(const ESValue& left, const ESValue& right);
//http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
NEVER_INLINE ESValue abstractRelationalComparison(const ESValue& left, const ESValue& right, bool leftFirst);
//d = {}. d[0]
NEVER_INLINE ESValue getObjectOperation(ESValue* willBeObject, ESValue* property, ESValue* lastObjectValueMetInMemberExpression, GlobalObject* globalObject);
//d = {}. d.foo
NEVER_INLINE ESValue getObjectPreComputedCaseOperation(ESValue* willBeObject, ESString* property, ESValue* lastObjectValueMetInMemberExpression, GlobalObject* globalObject
        ,ESHiddenClassChain* cachedHiddenClassChain, size_t* cachedHiddenClassIndex);
NEVER_INLINE ESValue getObjectOperationSlowMode(ESValue* willBeObject, ESValue* property, ESValue* lastObjectValueMetInMemberExpression, GlobalObject* globalObject);
//d = {}. d[0] = 1
NEVER_INLINE void setObjectOperation(ESValue* willBeObject, ESValue* property, const ESValue& value);
//d = {}. d.foo = 1
NEVER_INLINE void setObjectPreComputedCaseOperation(ESValue* willBeObject, ESString* keyString, const ESValue& value
        , ESHiddenClassChain* cachedHiddenClassChain, size_t* cachedHiddenClassIndex, ESHiddenClass** hiddenClassWillBe);
NEVER_INLINE void setObjectOperationSlowMode(ESValue* willBeObject, ESValue* property, const ESValue& value);
NEVER_INLINE bool instanceOfOperation(ESValue* lval, ESValue* rval);
NEVER_INLINE ESValue typeOfOperation(ESValue* v);
NEVER_INLINE ESValue newOperation(ESVMInstance* instance, GlobalObject* globalObject, ESValue fn, ESValue* arguments, size_t argc);

}
#endif
