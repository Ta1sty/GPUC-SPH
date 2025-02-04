#ifndef INCLUDE_SPATIAL_LOOKUP
#define INCLUDE_SPATIAL_LOOKUP

#extension GL_ARB_gpu_shader_int64: require

#include "_defines.glsl"

// only for syntax highlighting
#ifndef VEC_T
#define VEC_T vec3
#define UVEC_T uvec3
#define IVEC_T ivec3
#define SWIZZLE(v) ((v).xyz)
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

#ifndef GRID_BINDING_CACHE
#define GRID_BINDING_CACHE 3
#endif

#ifndef COORDINATES_BUFFER_NAME
#define COORDINATES_BUFFER_NAME particle_coordinates
#endif

struct SpatialLookupEntry {
	uint64_t data;
};

struct SpatialIndexEntry {
	uint start;
	uint end;
};

struct SpatialCacheEntry {
	uint cellKey;
	uint cellClass;
};

#ifdef GRID_PCR
 layout (push_constant) uniform PushStruct {
	int sceneType;
	float cellSize;
	uint numElements;
	uint sort_n;
	uint sort_k;
	uint sort_j;
} constants;

layout (set = GRID_SET, binding = GRID_BINDING_CACHE) buffer spatialCacheBuffer { SpatialCacheEntry spatial_cache[]; };

#endif

#ifdef GRID_WRITEABLE

layout (set = GRID_SET, binding = GRID_BINDING_LOOKUP) buffer spatialLookupBuffer { SpatialLookupEntry spatial_lookup[]; };
layout (set = GRID_SET, binding = GRID_BINDING_INDEX) buffer spatialIndexBuffer { SpatialIndexEntry spatial_indices[]; };

#if GRID_BINDING_COORDINATES > -1
 layout (set = GRID_SET, binding = GRID_BINDING_COORDINATES) buffer spatialParticleBuffer { VEC_T particle_coordinates[]; };
#endif

#else

layout (set = GRID_SET, binding = GRID_BINDING_LOOKUP) buffer readonly spatialLookupBuffer { SpatialLookupEntry spatial_lookup[]; };
layout (set = GRID_SET, binding = GRID_BINDING_INDEX) buffer readonly spatialIndexBuffer { SpatialIndexEntry spatial_indices[]; };

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

#define QUANTIZATION_BOUNDS 2.0f
#define QUANTIZATION_INDEX_BITS 23
#define QUANTIZATION_CLASS_BITS 5
#define QUANTIZATION_POSITION_BITS 12

const uint indexMask = (uint(1) << QUANTIZATION_INDEX_BITS) - 1;
const uint classMask = (uint(1) << QUANTIZATION_CLASS_BITS) - 1;
const uint positionMask = (uint(1) << QUANTIZATION_POSITION_BITS) - 1;
const uint quantizationRange = positionMask;

uint64_t quantize_index(uint index) {
	uint64_t value;

	if (index == -1) {
		value = indexMask;
	} else {
		value = (index & indexMask);
	}

	return value << 0;
}

uint64_t quantize_class(uint cellClass) {
	uint64_t value;

	if (cellClass == -1) {
		value = classMask;
	} else {
		value = (cellClass & classMask);
	}

	return value << QUANTIZATION_INDEX_BITS;
}

uint64_t quantize_position(VEC_T position) {
	// [-bounds,bounds]
	VEC_T normalized = ((position / QUANTIZATION_BOUNDS) + 1.0f) * 0.5f;
	// [0,1]
	UVEC_T quanitized = clamp(UVEC_T(round(normalized * float(quantizationRange))), 0, quantizationRange);
	// [0,range]

	uint64_t value = uint64_t(0);
	#ifdef DEF_3D
    value = (value | quanitized.z);
	value = value << QUANTIZATION_POSITION_BITS;
	#endif
    value = (value | quanitized.y);
	value = value << QUANTIZATION_POSITION_BITS;

	value = (value | quanitized.x);

	return value << (QUANTIZATION_CLASS_BITS + QUANTIZATION_INDEX_BITS);
}

uint64_t quanitize(uint index, uint cellClass, VEC_T position) {
	return quantize_position(position) | quantize_class(cellClass) | quantize_index(index);
}

uint dequantize_index(uint64_t data) {
	data = data >> 0;

	uint value = uint(data & indexMask);
	if (value == indexMask) {
		return -1;
	}
	return value;
}

uint dequantize_class(uint64_t data) {
	data = data >> QUANTIZATION_INDEX_BITS;

	uint value = uint(data & classMask);
	if (value == classMask) {
		return -1;
	}
	return value;
}

VEC_T dequantize_position(uint64_t data) {
	data = data >> (QUANTIZATION_CLASS_BITS + QUANTIZATION_INDEX_BITS);

	uint x = uint(data & positionMask);
	data = data >> QUANTIZATION_POSITION_BITS;
	uint y = uint(data & positionMask);

	#ifdef DEF_3D
    data = data >> QUANTIZATION_POSITION_BITS;
	uint z = uint(data & positionMask);
	UVEC_T quantized = UVEC_T(x, y, z);
	#endif

	#ifdef DEF_2D
    UVEC_T quantized = UVEC_T(x, y);
	#endif

	// [0,qMax]
	VEC_T normalized = VEC_T(quantized) / quantizationRange;
	// [0,1]
	VEC_T position = (normalized - 0.5) * 2 * QUANTIZATION_BOUNDS;
	// [-bounds,bounds]
	return position;
}


#ifdef DEF_2D
#define NEIGHBOUR_OFFSET_COUNT 9
const IVEC_T neighbourOffsets[NEIGHBOUR_OFFSET_COUNT] =       {
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

#include "spatial_lookup.traversal.glsl"

#endif