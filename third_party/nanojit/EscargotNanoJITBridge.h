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

#define FASTCALL

extern size_t pagesize;

size_t VMPI_getVMPageSize();

namespace nanojit {

#define REALLY_INLINE ALWAYS_INLINE

#if defined(AVMPLUS_IA32) || defined(AVMPLUS_AMD64)

#include <xmmintrin.h>

typedef __m128  float4_t;

#define f4_add    _mm_add_ps
#define f4_sub    _mm_sub_ps
#define f4_mul    _mm_mul_ps
#define f4_div    _mm_div_ps
#define f4_setall _mm_set_ps1
#define f4_sqrt   _mm_sqrt_ps

REALLY_INLINE int32_t f4_eq_i(float4_t a, float4_t b)
{
    return (_mm_movemask_epi8(_mm_castps_si128(_mm_cmpneq_ps(a, b))) == 0);
}

REALLY_INLINE float f4_x(float4_t v) { return _mm_cvtss_f32(v); }
REALLY_INLINE float f4_y(float4_t v) { return _mm_cvtss_f32(_mm_shuffle_ps(v, v, 0x55)); }
REALLY_INLINE float f4_z(float4_t v) { return _mm_cvtss_f32(_mm_shuffle_ps(v, v, 0xAA)); }
REALLY_INLINE float f4_w(float4_t v) { return _mm_cvtss_f32(_mm_shuffle_ps(v, v, 0xFF)); }

#elif defined(AVMPLUS_ARM)

#include <arm_neon.h>

typedef float32x4_t  float4_t;

#define f4_add       vaddq_f32
#define f4_sub       vsubq_f32
#define f4_mul       vmulq_f32
#define f4_sqrt      vsqrtq_f32
#define f4_setall    vdupq_n_f32

REALLY_INLINE int32_t f4_eq_i(float4_t a, float4_t b)
{
    uint32x2_t res = vreinterpret_u32_u16(vmovn_u32(vceqq_f32(a, b)));
    return vget_lane_u32(res, 0) == 0xffffffff && vget_lane_u32(res, 1) == 0xffffffff;
}

REALLY_INLINE float f4_x(float4_t v) { return vgetq_lane_f32(v, 0); }
REALLY_INLINE float f4_y(float4_t v) { return vgetq_lane_f32(v, 1); }
REALLY_INLINE float f4_z(float4_t v) { return vgetq_lane_f32(v, 2); }
REALLY_INLINE float f4_w(float4_t v) { return vgetq_lane_f32(v, 3); }

REALLY_INLINE float4_t f4_div(float4_t a, float4_t b)
{
    float4_t result = { f4_x(a) / f4_x(b), f4_y(a) / f4_y(b), f4_z(a) / f4_z(b), f4_w(a) / f4_w(b) };
    return result;
}

#else

typedef struct float4_t {
    float x, y, z, w;
} float4_t;

float4_t f4_add(const float4_t& x1, const float4_t& x2);
float4_t f4_sub(const float4_t& x1, const float4_t& x2);
float4_t f4_mul(const float4_t& x1, const float4_t& x2);
float4_t f4_div(const float4_t& x1, const float4_t& x2);

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

#endif

} // namespace nanojit

namespace avmplus {

void AvmLog(const char* fmt...);
void AvmAssertFail(const char *message);

} // namespace avmplus

#endif
