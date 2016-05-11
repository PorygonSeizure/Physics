#version 410

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texcoord;

out vec2 fragTexcoord;

uniform mat4 projectionView;

void main()
{
	fragTexcoord = texcoord;
	gl_Position = projectionView * position;
}