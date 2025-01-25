#ifndef INCLUDE_SPATIAL_LOOKUP
#define INCLUDE_SPATIAL_LOOKUP

#include "_defines.glsl"

#ifdef GRID_PCR
layout (push_constant) uniform PushStruct {
	int sceneType;
	float cellSize;
	uint numElements;
	uint sort_n;
	uint sort_k;
	uint sort_j;
} constants;
#endif

#ifndef GRID_NUM_ELEMENTS
#define GRID_NUM_ELEMENTS uint(constants.numElements)
#endif

#ifndef GRID_CELL_SIZE
#define GRID_CELL_SIZE float(constants.cellSize)
#endif

#ifndef GRID_SET
#define GRID_SET 0
#endif

#ifndef GRID_BINDING_LOOKUP
#define GRID_BINDING_LOOKUP 0
#endif

#ifndef GRID_BINDING_INDEX
#define GRID_BINDING_INDEX 1
#endif

#ifndef GRID_BINDING_COORDINATES
#define GRID_BINDING_COORDINATES 2
#endif

#ifndef COORDINATES_BUFFER_NAME
#define COORDINATES_BUFFER_NAME particle_coordinates
#endif

struct SpatialLookupEntry {
	uint cellKey;
	uint cellClass;
	uint particleIndex;
};

#ifdef GRID_WRITEABLE

layout (set = GRID_SET, binding = GRID_BINDING_LOOKUP) buffer spatialLookupBuffer { SpatialLookupEntry spatial_lookup[]; };
layout (set = GRID_SET, binding = GRID_BINDING_INDEX) buffer spatialIndexBuffer { uint spatial_indices[]; };

#if GRID_BINDING_COORDINATES > -1
layout (set = GRID_SET, binding = GRID_BINDING_COORDINATES) buffer spatialParticleBuffer { VEC_T particle_coordinates[]; };
#endif

#else

layout (set = GRID_SET, binding = GRID_BINDING_LOOKUP) buffer readonly spatialLookupBuffer { SpatialLookupEntry spatial_lookup[]; };
layout (set = GRID_SET, binding = GRID_BINDING_INDEX) buffer readonly spatialIndexBuffer { uint spatial_indices[]; };

#if GRID_BINDING_COORDINATES > -1
layout (set = GRID_SET, binding = GRID_BINDING_COORDINATES) buffer readonly spatialParticleBuffer { VEC_T particle_coordinates[]; };
#endif

#endif



IVEC_T cellCoord(VEC_T position) {
	return IVEC_T(position / GRID_CELL_SIZE);
}

uint cellHash(IVEC_T cell) {

	#ifdef DEF_2D
    return ((cell.x * 73856093) ^ (cell.y * 19349663));
	#endif

	#ifdef DEF_3D
    return ((cell.x * 73856093) ^ (cell.y * 19349663) ^ (cell.z * 83492791));
	#endif
}

uint cellClass(IVEC_T cell) {

	#ifdef DEF_2D
    return (3 * (cell.x % 3)) + (1 * (cell.y % 3));
	#endif

	#ifdef DEF_3D
    return (9 * (cell.x % 3)) + (3 * (cell.y % 3)) + (1 * (cell.z % 3));
	#endif
}

uint cellKey(uint hash) {
	return hash % GRID_NUM_ELEMENTS;
}

uint cellKey(IVEC_T cell) {
	return cellKey(cellHash(cell));
}

uint cellKey(VEC_T position) {
	return cellKey(cellHash(cellCoord(position)));
}

vec4 keyColor(uint cellKey) {
	return vec4(1.f * cellKey / GRID_NUM_ELEMENTS, 0, 0, 1);
}

vec4 classColor(uint classKey) {
	uint a = classKey % 3;
	classKey = classKey / 3;
	uint b = classKey % 3;
	classKey = classKey / 3;
	uint c = classKey % 3;

	return vec4(a / 3.0f, b / 3.0f, c / 3.0f, 1);
}


#ifdef DEF_2D
#define NEIGHBOUR_OFFSET_COUNT 9
const IVEC_T neighbourOffsets[NEIGHBOUR_OFFSET_COUNT] =             {
IVEC_T(- 1, - 1),
IVEC_T(- 1, 0),
IVEC_T(- 1, 1),
IVEC_T(0, - 1),
IVEC_T(0, 0),
IVEC_T(0, 1),
IVEC_T(1, - 1),
IVEC_T(1, 0),
IVEC_T(1, 1),
};

#endif

#ifdef DEF_3D
#define NEIGHBOUR_OFFSET_COUNT 27
const IVEC_T neighbourOffsets[NEIGHBOUR_OFFSET_COUNT] = {
IVEC_T(- 1, - 1, - 1),
IVEC_T(- 1, - 1, 0),
IVEC_T(- 1, - 1, 1),
IVEC_T(- 1, 0, - 1),
IVEC_T(- 1, 0, 0),
IVEC_T(- 1, 0, 1),
IVEC_T(- 1, 1, - 1),
IVEC_T(- 1, 1, 0),
IVEC_T(- 1, 1, 1),

IVEC_T(0, - 1, - 1),
IVEC_T(0, - 1, 0),
IVEC_T(0, - 1, 1),
IVEC_T(0, 0, - 1),
IVEC_T(0, 0, 0),
IVEC_T(0, 0, 1),
IVEC_T(0, 1, - 1),
IVEC_T(0, 1, 0),
IVEC_T(0, 1, 1),

IVEC_T(1, - 1, - 1),
IVEC_T(1, - 1, 0),
IVEC_T(1, - 1, 1),
IVEC_T(1, 0, - 1),
IVEC_T(1, 0, 0),
IVEC_T(1, 0, 1),
IVEC_T(1, 1, - 1),
IVEC_T(1, 1, 0),
IVEC_T(1, 1, 1),
};
#endif


#define NEIGHBOUR_INDEX n_index
#define NEIGHBOUR_POSITION n_position
#define NEIGHBOUR_DISTANCE n_distance
#define NEIGHBOUR_DISTANCE_SQUARED n_distance_squared

#define FOREACH_NEIGHBOUR(position, expression) { \
float radiusSquared = GRID_CELL_SIZE * GRID_CELL_SIZE; \
IVEC_T center = cellCoord(position); \
 for (int i = 0; i < NEIGHBOUR_OFFSET_COUNT; i++) { \
IVEC_T cellCoord = center + neighbourOffsets[i]; \
uint cellKey = cellKey(cellCoord); \
uint cellClass = cellClass(cellCoord); \
 for (uint j = spatial_indices[cellKey]; j < GRID_NUM_ELEMENTS; j++) { \
SpatialLookupEntry entry = spatial_lookup[j]; \
 if (entry.cellKey != cellKey) break; \
 if (entry.cellClass != cellClass) continue; \
uint NEIGHBOUR_INDEX = entry.particleIndex; \
 if (NEIGHBOUR_INDEX == uint(- 1)) continue; \
VEC_T NEIGHBOUR_POSITION = COORDINATES_BUFFER_NAME[NEIGHBOUR_INDEX]; \
VEC_T difference = position - NEIGHBOUR_POSITION; \
float NEIGHBOUR_DISTANCE_SQUARED = dot(difference, difference); \
 if (NEIGHBOUR_DISTANCE_SQUARED > radiusSquared) continue; \
float NEIGHBOUR_DISTANCE = sqrt(NEIGHBOUR_DISTANCE_SQUARED); \
expression; \
} \
} \
}

#endif