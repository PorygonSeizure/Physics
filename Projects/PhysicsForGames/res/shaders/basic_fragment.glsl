#version 410

in vec2 fragTexcoord;
out vec4 fragColor;

uniform sampler2D diffuse;

void main()
{
	frag_color = vec4(1, 1, 1, 1);	//texture(diffuse, fragTexcoord);
}