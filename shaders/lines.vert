#version 450

layout(push_constant) uniform PushStruct {
	mat4 mvp;    
    vec4 pos;
    float dT; // timestep
} p;

layout(binding = 0) buffer Alive {uint gAlive[];};
layout(binding = 1) buffer PosLife {vec4 gPosLife[];};
layout(binding = 2) buffer VelMass {vec4 gVelMass[];};
layout(binding = 3) readonly buffer TriangleSoup {vec4 gTriangleSoup[];};
layout(binding = 4) uniform sampler3D gForceFieldSampler;
layout(binding = 5) buffer Normals {vec4 gNormals[];};
layout(binding = 6) buffer ForceLines {vec4 gForceLines[];};

layout(location = 0) out vec3 color;

vec4 colorCode(float value)
{
	value *= 0.2;

	vec4 retVal;
	retVal.x = 2.0 * value * mix(0.0, 1.0, max(0.0, 1.0 - value)); 
	retVal.y = 2.0 * value * min( mix(0.0, 1.0, max(0.0, value) / 0.5), mix(0.0, 1.0, max(0.0, (1.0 - value) / 0.5))); 
	retVal.z = 1.f;
	retVal.w = 0.4;
	
	return retVal;
}
/*
void main() {
    uint i = gl_VertexIndex;
    gl_PointSize = 1.f;
    if (gAlive[i] == 0) {
        gl_Position = vec4(0.f / 0.f);
        return;
    }
    vec4 posLife = gPosLife[i];
    
    color = vec3(0.1, 1, 0.01);
    if(posLife.w<1.f)
        color=colorCode(posLife.w).xyz;
    color=texture(gForceFieldSampler, posLife.xyz).xyz;
    gl_Position = p.mvp * vec4(posLife.xyz, 1);
}
*/

const uint dim = 10;

void main() {
    if (false) {
        color = vec3(0.5, 0.01, 0.001);
        if (gl_VertexIndex % 2 == 0) {
            gl_Position = p.mvp * vec4(0, 0, 0, 1);
        } else {
            gl_Position = p.mvp * vec4(1, 1, 1, 1);
        }
        return;
    }

    uint i = gl_VertexIndex / 3;
    uint o = min(gl_VertexIndex % 3, 1);

    uint x = i % dim;
    uint yz = i / dim;
    uint y = yz / dim;
    uint z = yz % dim;

    vec3 lookUp = (vec3(x, y, z) + 0.5) / dim;
    vec4 force = texture(gForceFieldSampler, lookUp);
        
    color = vec3(o, 1 - o, 1);
    
    if(o == 0)
        lookUp += 0.003 * force.xyz;
            
    gl_Position = p.mvp * vec4(lookUp, 1);
}
