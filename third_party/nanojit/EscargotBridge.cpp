#include "EscargotNanoJITBridge.h"
#include "nanojit.h"

size_t pagesize = size_t(sysconf(_SC_PAGESIZE));

size_t VMPI_getVMPageSize()
{
    return pagesize;
}

namespace nanojit {

#ifdef DEBUG
void ValidateWriter::checkAccSet(LOpcode op, LIns *base, int32_t disp, AccSet accSet)
{
    // TODO
}
#endif

#if !defined(AVMPLUS_IA32) && !defined(AVMPLUS_AMD64) && !defined(AVMPLUS_ARM)
float4_t f4_add(const float4_t& x1, const float4_t& x2)
{
    float4_t retval = { x1.x + x2.x, x1.y + x2.y, x1.z + x2.z, x1.w + x2.w };
    return retval;
}

float4_t f4_sub(const float4_t& x1, const float4_t& x2)
{
    float4_t retval = { x1.x - x2.x, x1.y - x2.y, x1.z - x2.z, x1.w - x2.w };
    return retval;
}

float4_t f4_mul(const float4_t& x1, const float4_t& x2)
{
    float4_t retval = { x1.x * x2.x, x1.y * x2.y, x1.z * x2.z, x1.w * x2.w };
    return retval;
}

float4_t f4_div(const float4_t& x1, const float4_t& x2)
{
    float4_t retval = { x1.x / x2.x, x1.y / x2.y, x1.z / x2.z, x1.w / x2.w };
    return retval;
}
#endif

}

namespace avmplus {

void AvmLog(const char* fmt...)
{
#ifndef NDEBUG
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
#endif
}

void AvmAssertFail(const char *message)
{
    AvmLog(message);
    ASSERT(false);
}

}
