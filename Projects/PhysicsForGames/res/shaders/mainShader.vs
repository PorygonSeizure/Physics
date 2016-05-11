#version 410

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

out vec3 fragNormal;
out vec3 fragPosition;
out vec3 fragTangent;
out vec3 fragBitangent;
out vec2 fragTexcoord;

uniform mat4 viewProj;
uniform mat4 model;
uniform mat4 modelViewProj;

void main()
{
	fragPosition = (model * vec4(position, 1)).xyz;
    fragNormal = (model * vec4(normal, 1)).xyz;
	fragTangent = (model * vec4(tangent, 1)).xyz;
	fragBitangent = (model * vec4(bitangent, 1)).xyz;

	fragTexcoord = texCoord;
	
	gl_Position = modelViewProj * vec4(position, 1);
}
