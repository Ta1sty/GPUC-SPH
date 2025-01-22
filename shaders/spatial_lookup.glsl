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

#ifndef GRID_READONLY
#define GRID_READONLY readonly
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
uint particleIndex;
};

layout (set = GRID_SET, binding = GRID_BINDING_LOOKUP) buffer GRID_READONLY spatialLookupBuffer { SpatialLookupEntry spatial_lookup[]; };
layout (set = GRID_SET, binding = GRID_BINDING_INDEX) buffer GRID_READONLY spatialIndexBuffer { uint spatial_indices[]; };

#if GRID_BINDING_COORDINATES > - 1
 layout (set = GRID_SET, binding = GRID_BINDING_COORDINATES) buffer spatialParticleBuffer { VEC_T particle_coordinates[]; };
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

uint cellKey(uint hash){
return hash % GRID_NUM_ELEMENTS;
}

uint cellKey(IVEC_T cell){
return cellKey(cellHash(cell));
}

uint cellKey(VEC_T position){
return cellKey(cellHash(cellCoord(position)));
}

vec4 cellColor(uint cellKey) {
return vec4(1.f * cellKey / GRID_NUM_ELEMENTS, 0, 0, 1);
}


#ifdef DEF_2D
#define NEIGHBOUR_OFFSET_COUNT 9
 const IVEC_T neighbourOffsets[NEIGHBOUR_OFFSET_COUNT] = {
IVEC_T(- 1, - 1),
IVEC_T(- 1, 0),
IVEC_T(-1, 1),
IVEC_T(0, -1),
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

#define FOREACH_NEIGHBOUR(position, expression) { \
IVEC_T center = cellCoord(position); \
for (int i = 0; i < NEIGHBOUR_OFFSET_COUNT; i++) { \
uint cellKey = cellKey(center + neighbourOffsets[i]); \
for (uint j = spatial_indices[cellKey]; j < GRID_NUM_ELEMENTS; j++) { \
SpatialLookupEntry entry = spatial_lookup[j]; \
if (entry.cellKey != cellKey) break; \
uint NEIGHBOUR_INDEX = entry.particleIndex; \
if (NEIGHBOUR_INDEX == uint(- 1)) continue; \
VEC_T NEIGHBOUR_POSITION = COORDINATES_BUFFER_NAME[NEIGHBOUR_INDEX]; \
float NEIGHBOUR_DISTANCE = length(position - NEIGHBOUR_POSITION); \
if (NEIGHBOUR_DISTANCE > GRID_CELL_SIZE) continue; \
expression; \
} \
} \
}

#endif