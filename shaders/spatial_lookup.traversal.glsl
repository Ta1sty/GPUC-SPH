#ifndef INCLUDE_SPATIAL_LOOKUP_TRAVERSAL
#define INCLUDE_SPATIAL_LOOKUP_TRAVERSAL

// only for syntax highlighting
#ifndef VEC_T
#define VEC_T vec3
#define UVEC_T uvec3
#define IVEC_T ivec3
#define SWIZZLE(v) ((v).xyz)
#endif

#define NEIGHBOUR_INDEX n_index
#define NEIGHBOUR_POSITION n_position
#define NEIGHBOUR_DISTANCE n_distance
#define NEIGHBOUR_DISTANCE_SQUARED n_distance_squared

#define FOREACH_NEIGHBOUR(position, expression) { \
float radiusSquared = GRID_CELL_SIZE * GRID_CELL_SIZE; \
IVEC_T center = cellCoord(position); \
 for (int i = 0; i < NEIGHBOUR_OFFSET_COUNT; i++) {\
IVEC_T pCell = center + neighbourOffsets[i]; \
uint pKey = cellKey(pCell); \
uint pClass = cellClass(pCell); \
bool foundClass = false; \
SpatialIndexEntry spatial_index = spatial_indices[pKey]; \
 for (uint j = spatial_index.start; j < spatial_index.end; j++) {\
uint64_t lookup = spatial_lookup[j].data; \
uint nClass = dequantize_class(lookup); \
 if (pClass != nClass) {\
 if (foundClass) break; \
 continue; \
}\
foundClass = true; \
VEC_T NEIGHBOUR_POSITION = dequantize_position(lookup); \
VEC_T difference = position - NEIGHBOUR_POSITION; \
float NEIGHBOUR_DISTANCE_SQUARED = dot(difference, difference); \
 if (NEIGHBOUR_DISTANCE_SQUARED > radiusSquared) continue; \
float NEIGHBOUR_DISTANCE = sqrt(NEIGHBOUR_DISTANCE_SQUARED); \
\
uint NEIGHBOUR_INDEX = dequantize_index(lookup); \
{expression; } \
}\
}\
}

#endif