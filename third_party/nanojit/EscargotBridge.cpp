#include "EscargotNanoJITBridge.h"

size_t VMPI_getVMPageSize()
{
    return pagesize;
}

namespace nanojit {

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

}

namespace avmplus {

void AvmLog(const char* fmt...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, fmt, args);
    va_end(args);
}

void AvmAssertFail(const char *message)
{
    AvmLog(message);
    ASSERT(false);
}

}
