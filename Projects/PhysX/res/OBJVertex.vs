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
out vec4 vertexShadowCoord;

uniform mat4 projectionView;
uniform mat4 lightMatrix;
uniform vec3 offset;
uniform vec3 colourOffset;

void main()
{
	vertexPosition = vec4(position.x + offset.x, position.y + offset.y, position.z + offset.z, position.w);
	vertexColour = vec4(colourOffset.x, colourOffset.y, colourOffset.z, 1);
	vertexNormal = normal;
	vertexTangent = tangent;
	vertexTexCoord = texCoord;
	vertexBiTangent = cross(normal.xyz, tangent.xyz);
	vertexShadowCoord = lightMatrix * vertexPosition;

	gl_Position = projectionView * vertexPosition;
}