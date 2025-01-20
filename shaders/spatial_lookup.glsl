#ifdef GRID_PCR
layout (push_constant) uniform PushStruct {
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

 struct SpatialLookupEntry {
uint cellKey;
uint particleIndex;
};

layout (set = GRID_SET, binding = GRID_BINDING_LOOKUP) buffer GRID_READONLY spatialLookupBuffer { SpatialLookupEntry spatial_lookup[]; };
layout (set = GRID_SET, binding = GRID_BINDING_INDEX) buffer GRID_READONLY spatialIndexBuffer { uint spatial_indices[]; };

#if GRID_BINDING_COORDINATES > - 1
 layout (set = GRID_SET, binding = GRID_BINDING_COORDINATES) buffer spatialParticleBuffer { vec2 particle_coordinates[]; };
#endif

ivec2 cellCoord(vec2 position) {
return ivec2(int(position.x / GRID_CELL_SIZE), int(position.y / GRID_CELL_SIZE));
}

uint cellHash(ivec2 cell) {

uint key = ((cell.x * 73856093) ^ (cell.y * 19349663));

return key;
}

uint cellKey(uint hash){
return hash % GRID_NUM_ELEMENTS;
}

uint cellKey(ivec2 cell){
return cellKey(cellHash(cell));
}

uint cellKey(vec2 position){
return cellKey(cellHash(cellCoord(position)));
}

vec4 cellColor(uint cellKey) {
return vec4(1.f * cellKey / GRID_NUM_ELEMENTS, 0, 0, 1);
}

#define NEIGHBOUR_OFFSET_COUNT 9

 const ivec2 offsets[NEIGHBOUR_OFFSET_COUNT] = {
ivec2(- 1, - 1),
ivec2(- 1, 0),
ivec2(-1, 1),
ivec2(0, -1),
ivec2(0, 0),
ivec2(0, 1),
ivec2(1, - 1),
ivec2(1, 0),
ivec2(1, 1),
};

#define NEIGHBOUR_INDEX n_index
#define NEIGHBOUR_POSITION n_position
#define NEIGHBOUR_DISTANCE n_distance

#define FOREACH_NEIGHBOUR(position, expression) { \
ivec2 center = cellCoord(position); \
for (int i = 0; i < NEIGHBOUR_OFFSET_COUNT; i++) { \
uint cellKey = cellKey(center + offsets[i]); \
for (uint j = spatial_indices[cellKey]; j < GRID_NUM_ELEMENTS; j++) { \
SpatialLookupEntry entry = spatial_lookup[j]; \
if (entry.cellKey != cellKey) break; \
uint NEIGHBOUR_INDEX = entry.particleIndex; \
if (NEIGHBOUR_INDEX == uint(- 1)) continue; \
vec2 NEIGHBOUR_POSITION = coordinates[NEIGHBOUR_INDEX]; \
float NEIGHBOUR_DISTANCE = length(position - NEIGHBOUR_POSITION); \
if (NEIGHBOUR_DISTANCE > GRID_CELL_SIZE) continue; \
expression; \
} \
} \
}