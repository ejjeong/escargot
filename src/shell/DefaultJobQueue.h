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
#ifndef DefaultJobQueue_h
#define DefaultJobQueue_h

#include "Escargot.h"
#include "runtime/JobQueue.h"

namespace escargot {

class ESVMInstance;

class DefaultJobQueue : public JobQueue {
private:
    DefaultJobQueue() { }

public:
    static DefaultJobQueue* create()
    {
        return new DefaultJobQueue();
    }

    size_t enqueueJob(Job* job)
    {
        // ESCARGOT_LOG_INFO("=== enqueue job %p\n", job);
        m_jobs.push_back(job);
        return 0;
    }

    bool hasNextJob()
    {
        return !m_jobs.empty();
    }

    Job* nextJob()
    {
        ASSERT(!m_jobs.empty());
        Job* job = m_jobs.front();
        m_jobs.pop_front();
        return job;
    }

    static DefaultJobQueue* get(JobQueue* jobQueue)
    {
        return (DefaultJobQueue*)jobQueue;
    }

private:
    std::list<Job*, gc_allocator<Job*> > m_jobs;

};

JobQueue* JobQueue::create()
{
    return escargot::DefaultJobQueue::create();
}

}
#endif
#endif
