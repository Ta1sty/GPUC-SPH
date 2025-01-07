#ifdef GRID_PCR
layout(push_constant) uniform PushStruct {
    uint size;
} constants;
#endif

#ifndef GRID_SIZE
#define GRID_SIZE uint(constants.size)
#endif

struct SpatialLookupEntry {
    uint cellKey;
    uint particleIndex;
};

layout (binding = 0) buffer lookupBuffer { SpatialLookupEntry spatial_lookup[]; };
layout (binding = 1) buffer indexBuffer { uint spatial_indices[]; };
layout (binding = 2) buffer particleBuffer { vec2 particles_coordinates[]; };

uint cellHash(uvec2 cell) {
    return ((cell.x * 73856093) ^ (cell.y * 19349663)) % GRID_SIZE;
}