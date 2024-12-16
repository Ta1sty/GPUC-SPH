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

layout(location = 0) out vec3 color;

vec4 colorCode(float value)
{
	value *= 0.2f;

	vec4 retVal;
	retVal.x = 0.5f; 
	retVal.y = 2.0f * value * mix(0.f, 1.f, max(0.f, 1.f - value)); 
	retVal.z = 2.0f * value * min( mix(0.f, 1.f, max(0.f, value) / 0.5f), mix(0.f, 1.f, max(0.f, (1.f - value) / 0.5f)));
	retVal.w = 0.4f;
	
	return retVal;
}

void main() {
    uint i = gl_VertexIndex;
    gl_PointSize = 3.f;
    if (gAlive[i] == 0) {
        gl_Position = vec4(0.f / 0.f);
        return;
    }
    vec4 posLife = gPosLife[i];
    
    color = vec3(0.1, 1, 0.01);
    if(posLife.w<1.5f)
        color=colorCode(posLife.w).xyz;
    gl_Position = p.mvp * vec4(posLife.xyz, 1);
}