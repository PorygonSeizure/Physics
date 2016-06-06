#version 410

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 colour;
layout(location = 2) in vec4 normal;
layout(location = 3) in vec4 tangent;
layout(location = 4) in vec2 texCoord;

out vec4 vertexPosition;
out vec4 vertexColour;
out vec4 vertexNormal;
out vec4 vertexTangent;
out vec2 vertexTexCoord;
out vec3 vertexBiTangent;

uniform mat4 projectionView;
uniform vec3 offset;

void main()
{
	vertexPosition = vec4(position.x + offset.x, position.y + offset.y, position.z + offset.z, position.w);
	vertexColour = vec4(colour.x, colour.y, colour.z, 1);
	vertexNormal = normal;
	vertexTangent = tangent;
	vertexTexCoord = texCoord;
	vertexBiTangent = cross(normal.xyz, tangent.xyz);

	gl_Position = projectionView * vertexPosition;
}