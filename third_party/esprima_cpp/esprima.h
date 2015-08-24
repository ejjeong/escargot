#ifndef ESPRIMA_H
#define ESPRIMA_H

#include "Escargot.h"

namespace escargot {
    class Node;
}
namespace esprima {

escargot::Node* parse(const escargot::u16string& source);

}

#endif
