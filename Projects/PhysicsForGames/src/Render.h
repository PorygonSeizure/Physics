#ifndef _RENDER_H_
#define _RENDER_H_

#define GLM_SWIZZLE
#include "glm/glm.hpp"

#include <vector>

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;

struct Material
{
    int diffuseTexture;
    int normalTexture;
    int specularTexture;

    vec3 diffuseColor;
    vec3 specularColor;
};

struct SimpleVertex
{
    vec3 pos;
    vec3 normal;
    vec2 uv;
};

struct SubMesh
{
	Material material;

    unsigned int ibo;
    unsigned int vao;
	
	unsigned int* indexData;
	unsigned int indexCount;
};

struct Mesh
{
	unsigned int subMeshCount;
	SubMesh* subMeshes;

	unsigned int vbo;

	unsigned int vertexCount;
	SimpleVertex* vertexData;
};

struct Scene
{
	Mesh* meshes;
	unsigned int meshCount;
};


Scene LoadSceneFromOBJ(char* dir, char* filename);
void FreeSceneFromOBJ(Scene* scene);

unsigned int LoadGLTextureBasic(const char * path);


Mesh* CreateMeshFromBuffers(SimpleVertex* vertexData, unsigned int vertexCount, unsigned int* indexData, unsigned int indexCount, Material material);
//NOTE(aidan): This should only be called on meshes created from the
//CreateMeshFromBuffers function. DO NOT call on the meshes in a scene
void FreeMesh(Mesh* mesh);
void RebuildVertexBuffer(Mesh* mesh);

//class Shader;

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void PushMesh(Mesh* mesh, mat4 transform);
    void RenderAndClear(mat4 viewProj);

	struct RenderItem
	{
		Mesh* mesh;
		mat4 transform;
	};

	std::vector<RenderItem> renderQueue;

    unsigned int mainShader;
	//Shader* mainShader;
};

#endif	