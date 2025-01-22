#ifndef INCLUDE_DEFINES
#define INCLUDE_DEFINES

#ifdef DEF_2D
#define VEC_T vec2
#define UVEC_T uvec2
#define IVEC_T ivec2
#define SWIZZLE(v) ((v).xy)
#endif

#ifdef DEF_3D
#define VEC_T vec3
#define UVEC_T uvec3
#define IVEC_T ivec3
#define SWIZZLE(v) ((v).xyz)
#endif

#endif