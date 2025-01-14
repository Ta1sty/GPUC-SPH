#ifdef GRID_PCR
layout (push_constant) uniform PushStruct {
    uint bufferSize;
    float cellSize;
} constants;
#endif

#ifndef GRID_BUFFER_SIZE
#define GRID_BUFFER_SIZE uint(constants.bufferSize)
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

layout (set = GRID_SET, binding = GRID_BINDING_LOOKUP) buffer spatialLookupBuffer { SpatialLookupEntry spatial_lookup[]; };
layout (set = GRID_SET, binding = GRID_BINDING_INDEX) buffer spatialIndexBuffer { uint spatial_indices[]; };

#if GRID_BINDING_COORDINATES > -1
layout (set = GRID_SET, binding = GRID_BINDING_COORDINATES) buffer spatialParticleBuffer { vec2 particle_coordinates[]; };
#endif

uint cellKey(vec2 position) {
    float inverseSize = 1.f / GRID_CELL_SIZE;
    vec2 scaled = vec2(position.x * inverseSize, position.y * inverseSize);
    uvec2 cell = uvec2(scaled);

    uint key = ((cell.x * 73856093) ^ (cell.y * 19349663));
    return key % GRID_BUFFER_SIZE;
}

vec4 cellColor(uint cellKey) {
    return vec4(1.f * cellKey / GRID_BUFFER_SIZE, 0, 0, 1);
}