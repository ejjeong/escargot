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

#ifndef ExpressionNode_h
#define ExpressionNode_h

#include "Node.h"
#include "PatternNode.h"

namespace escargot {

typedef std::vector<Node *, gc_allocator<Node *>> ArgumentVector;

// Any expression node. Since the left-hand side of an assignment may be any expression in general, an expression can also be a pattern.
// interface Expression <: Node, Pattern { }
class ExpressionNode : public Node {
public:
    ExpressionNode(NodeType type)
        : Node(type)
    {
    }
protected:
};

typedef std::vector<Node *, gc_allocator<Node *>> ExpressionNodeVector;

}

#endif
