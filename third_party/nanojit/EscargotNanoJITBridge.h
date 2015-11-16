#ifndef EscargotNanoJITBridge_h
#define EscargotNanoJITBridge_h

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include "Escargot.h"

#define VMPI_memcpy         ::memcpy
#define VMPI_memset         ::memset
#define VMPI_memcmp         ::memcmp
#define VMPI_memmove        ::memmove
#define VMPI_memchr         ::memchr
#define VMPI_strcmp         ::strcmp
#define VMPI_strcat         ::strcat
#define VMPI_strchr         ::strchr
#define VMPI_strrchr        ::strrchr
#define VMPI_strcpy         ::strcpy
#define VMPI_strlen         ::strlen
#define VMPI_strncat        ::strncat
#define VMPI_strncmp        ::strncmp
#define VMPI_strncpy        ::strncpy
#define VMPI_strtol         ::strtol
#define VMPI_strstr         ::strstr

#define VMPI_sprintf        ::sprintf
#define VMPI_snprintf       ::snprintf
#define VMPI_vsnprintf      ::vsnprintf
#define VMPI_sscanf         ::sscanf

#define VMPI_atoi   ::atoi
#define VMPI_tolower ::tolower
#define VMPI_islower ::islower
#define VMPI_toupper ::toupper
#define VMPI_isupper ::isupper
#define VMPI_isdigit ::isdigit
#define VMPI_isalnum ::isalnum
#define VMPI_isxdigit ::isxdigit
#define VMPI_isspace ::isspace
#define VMPI_isgraph ::isgraph
#define VMPI_isprint ::isprint
#define VMPI_ispunct ::ispunct
#define VMPI_iscntrl ::iscntrl
#define VMPI_isalpha ::isalpha
#define VMPI_abort   ::abort
#define VMPI_exit    ::exit

# define PERFM_NVPROF(n,v)
# define PERFM_NTPROF_BEGIN(n)
# define PERFM_NTPROF_END(n)

#define _nvprof(e,v)

extern size_t pagesize;

size_t VMPI_getVMPageSize();

namespace nanojit {

typedef struct float4_t {
    float x, y, z, w;
} float4_t;

float4_t f4_add(const float4_t& x1, const float4_t& x2);
float4_t f4_sub(const float4_t& x1, const float4_t& x2);
float4_t f4_mul(const float4_t& x1, const float4_t& x2);
float4_t f4_div(const float4_t& x1, const float4_t& x2);

#define REALLY_INLINE ALWAYS_INLINE

REALLY_INLINE int32_t f4_eq_i(const float4_t& x1, const float4_t& x2)
{
    return (x1.x == x2.x) && (x1.y == x2.y) && (x1.z == x2.z) && (x1.w == x2.w);
}

REALLY_INLINE float4_t f4_setall(float v)
{
    float4_t retval = {v, v, v, v};
    return retval;
}

REALLY_INLINE float4_t f4_sqrt(const float4_t& v)
{
    float4_t retval = { sqrtf(v.x), sqrtf(v.y), sqrtf(v.z), sqrtf(v.w)};
    return retval;
}

REALLY_INLINE float f4_x(const float4_t& v) { return v.x; }
REALLY_INLINE float f4_y(const float4_t& v) { return v.y; }
REALLY_INLINE float f4_z(const float4_t& v) { return v.z; }
REALLY_INLINE float f4_w(const float4_t& v) { return v.w; }

} // namespace nanojit

namespace avmplus {

void AvmLog(const char* fmt...);
void AvmAssertFail(const char *message);

} // namespace avmplus

#endif
