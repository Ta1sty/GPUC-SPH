#version 450

layout(push_constant) uniform PushStruct {
	mat4 mvp;
	vec4 viewPos;
	float dT; // timestep	
} p;

layout(binding = 3) readonly buffer TriangleSoup {vec4 gTriangleSoup[];};
layout(binding = 5) readonly buffer NormalSoup {vec4 gNormals[];};

layout(location = 0) out vec3 pos;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 albedo;

void main() {
	pos = gTriangleSoup[gl_VertexIndex].xyz;
	normal = gNormals[gl_VertexIndex].xyz;
	albedo = vec3(0.5);
	gl_Position = p.mvp * vec4(pos, 1);

}