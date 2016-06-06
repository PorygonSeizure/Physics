#version 410

uniform vec3 lightColour = vec3(1, 1, 1);
uniform float lightAmbient;
uniform float lightDiffuse;
uniform float specularPower;

in vec4 vertexPosition;
in vec4 vertexColour;
in vec4 vertexNormal;
in vec4 vertexTangent;
in vec2 vertexTexCoord;
in vec3 vertexBiTangent;

uniform vec3 cameraPosition;
uniform vec3 lightPosition;

uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;
uniform sampler2D specularTexture;

out vec4 fragColor;

void main()
{
	vec3 lightAmbientColour = lightColour * lightAmbient;
	vec3 lightDiffuseColour = lightColour * lightDiffuse;
	vec3 lightSpecularColour = lightColour;

	vec3 newLightDirection = normalize(lightPosition - vertexPosition.xyz);

	mat3 TBN = mat3(normalize(vertexTangent.xyz), normalize(vertexBiTangent), normalize(vertexNormal.xyz));
	vec3 n = texture(normalTexture, vertexTexCoord).xyz * 2 - 1;

	vec3 ambient = vertexColour.rgb * lightAmbientColour * texture(diffuseTexture, vertexTexCoord).rgb;

	float d = max(0.0f, dot(normalize(TBN * n), -newLightDirection));

	vec3 diffuse = vertexColour.rgb * lightDiffuseColour * d * texture(diffuseTexture, vertexTexCoord).rgb;

	vec3 r = reflect(newLightDirection, normalize(vertexNormal.xyz));
	vec3 e = normalize(cameraPosition - vertexPosition.xyz);

	float spec = pow(max(0.0f, dot(r, e)), specularPower);
	vec3 specular = vertexColour.rgb * lightSpecularColour * spec * texture(specularTexture, vertexTexCoord).rgb;

	fragColor = vec4(ambient + diffuse + specular, 1);
}