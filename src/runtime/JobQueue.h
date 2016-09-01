/*
 *  Copyright (C) 2016 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifdef USE_ES6_FEATURE
#ifndef JobQueue_h
#define JobQueue_h

#include "Escargot.h"

namespace escargot {

class ESVMInstance;

class Job : public gc {
public:
    enum JobType {
        ScriptJob,
        PromiseReactionJob,
        PromiseResolveThenableJob,
    };

protected:
    Job(JobType type)
        : m_type(type)
    {
    }

public:
    static Job* create()
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual ESValue run(ESVMInstance* instance) = 0;

private:
    JobType m_type;
};

class PromiseReactionJob : public Job {
private:
    PromiseReactionJob(escargot::PromiseReaction reaction, ESValue argument)
        : Job(JobType::PromiseReactionJob)
        , m_reaction(reaction)
        , m_argument(argument)
    {
    }

public:
    static Job* create(escargot::PromiseReaction reaction, ESValue argument)
    {
        return new PromiseReactionJob(reaction, argument);
    }

    ESValue run(ESVMInstance* instance)
    {
        // ESCARGOT_LOG_INFO("=== Running PromiseReactionJob %p\n", this);

        /* 25.4.2.1.4 Handler is "Identity" case */
        if (m_reaction.m_handler == (escargot::ESFunctionObject*)1) {
            // ESCARGOT_LOG_INFO("Identity case\n");
            ESValue value[] = { m_argument };
            return ESFunctionObject::call(instance, m_reaction.m_capability.m_resolveFunction, ESValue(), value, 1, false);
        }

        /* 25.4.2.1.5 Handler is "Thrower" case */
        if (m_reaction.m_handler == (escargot::ESFunctionObject*)2) {
            // ESCARGOT_LOG_INFO("Thrower case\n");
            ESValue value[] = { m_argument };
            return ESFunctionObject::call(instance, m_reaction.m_capability.m_rejectFunction, ESValue(), value, 1, false);
        }

        std::jmp_buf tryPosition;
        if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
            ESValue arguments[] = { m_argument };
            ESValue res = escargot::ESFunctionObject::call(instance, m_reaction.m_handler, ESValue(), arguments, 1, false);
            instance->unregisterTryPos(&tryPosition);
            instance->unregisterCheckedObjectAll();

            ESValue value[] = { res };
            return ESFunctionObject::call(instance, m_reaction.m_capability.m_resolveFunction, ESValue(), value, 1, false);
        } else {
            escargot::ESValue err = instance->getCatchedError();
            ESValue reason[] = { err };
            return ESFunctionObject::call(instance, m_reaction.m_capability.m_rejectFunction, ESValue(), reason, 1, false);
        }
    }

private:
    escargot::PromiseReaction m_reaction;
    ESValue m_argument;
};

class PromiseResolveThenableJob : public Job {
private:
    PromiseResolveThenableJob(escargot::ESPromiseObject* promise, escargot::ESObject* thenable, escargot::ESFunctionObject* then)
        : Job(JobType::PromiseResolveThenableJob)
        , m_promise(promise)
        , m_thenable(thenable)
        , m_then(then)
    {
    }

public:
    static Job* create(escargot::ESPromiseObject* promise, escargot::ESObject* thenable, escargot::ESFunctionObject* then)
    {
        return new PromiseResolveThenableJob(promise, thenable, then);
    }

    ESValue run(ESVMInstance* instance)
    {
        // ESCARGOT_LOG_INFO("=== [Promise %p] Running PromiseResolveThenableJob %p\n", m_promise, this);

        escargot::ESFunctionObject* promiseResolveFunction = nullptr;
        escargot::ESFunctionObject* promiseRejectFunction = nullptr;
        m_promise->createResolvingFunctions(instance, promiseResolveFunction, promiseRejectFunction);

        std::jmp_buf tryPosition;
        if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
            ESValue arguments[] = { promiseResolveFunction, promiseRejectFunction };
            ESValue thenCallResult = escargot::ESFunctionObject::call(instance, m_then, m_thenable, arguments, 1, false);
            instance->unregisterTryPos(&tryPosition);
            instance->unregisterCheckedObjectAll();

            ESValue value[] = { thenCallResult };
            return ESValue();
        } else {
            escargot::ESValue err = instance->getCatchedError();

            escargot::ESObject* alreadyResolved = ESPromiseObject::resolvingFunctionAlreadyResolved(promiseResolveFunction);
            if (alreadyResolved->get(strings->value.string()).asBoolean())
                return ESValue();
            alreadyResolved->set(strings->value.string(), ESValue(true));

            ESValue reason[] = { err };
            return ESFunctionObject::call(instance, promiseRejectFunction, ESValue(), reason, 1, false);
        }
    }

private:
    escargot::ESPromiseObject* m_promise;
    escargot::ESObject* m_thenable;
    escargot::ESObject* m_then;
};


class ScriptJob : public Job {
};

class JobQueue : public gc {
protected:
    JobQueue() { }
public:
    static JobQueue* create();
    virtual size_t enqueueJob(Job* job) = 0;
};

}
#endif
#endif
