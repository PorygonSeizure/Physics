#version 410

in vec3 fragNormal;
in vec3 fragPosition;
in vec3 fragTangent;
in vec3 fragBitangent;
in vec2 fragTexcoord;

out vec4 fragColor;

uniform vec3 ambientLight;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform float specularPower;

uniform vec3 eyePos;
uniform vec3 cameraDir;

uniform sampler2D diffuseTex;
uniform sampler2D normalTex;
uniform sampler2D specularTex;

void main()
{
	mat3 TBN = mat3(normalize(fragTangent), normalize(fragBitangent), normalize(fragNormal));
	
	vec3 sampledNormal = texture(normalTex, fragTexcoord).xyz;
	vec3 adjustedNormal = sampledNormal * 2 - 1;

	vec3 N = normalize(TBN * adjustedNormal);

	vec3 materialColor = texture(diffuseTex, vec2(fragTexcoord.x * 2, 1) - fragTexcoord).xyz;

	vec3 L = normalize(lightDir);

	vec3 ambient = materialColor * ambientLight;

	float d = max(0.0, dot(N, -L));
	vec3 diffuse = vec3(d) * lightColor * materialColor;

	vec3 E = normalize(eyePos - fragPosition);
	vec3 R = reflect(L, N);

	float s = max(0, dot(R, E));

	vec3 materialSpecular = texture(specularTex, fragTexcoord).xyz;
	s = pow(s, specularPower);
	vec3 specular = vec3(s) * lightColor * materialSpecular;
    
	fragColor = vec4(materialColor, 1);	//vec4(ambient + diffuse, 1);
}