#ifndef _MESH_H_
#define _MESH_H_

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <vector>

//a vertex structure that can be used with all tutorials
struct Vertex
{
	glm::vec4 position;	//added to attrib location 0
	glm::vec4 colour;	//added to attrib location 1
	glm::vec4 normal;	//added to attrib location 2
	glm::vec4 tangent;	//added to attrib location 4

	glm::vec2 texcoord;	//added to attrib location 3

	//animation data
	float boneWeights[4];	//added to attrib location 5
	int boneIDs[4];	//added to attrib location 6
};

//a simple wrapper for a texture filename and OpenGL handle
class Texture
{
public:
	Texture() : m_width(0), m_height(0), m_format(0), m_handle(0) {}
	~Texture();

	bool LoadTexture(const char* path);

	std::string m_filename;

	int m_width;
	int m_height;

	unsigned int m_format;
	unsigned int m_handle;
};

//a basic material
class Material
{
public:
	Material() : m_ambient(1), m_diffuse(1), m_specular(0), m_emissive(0), m_specularPower(1), m_opacity(1) {}
	~Material() {}

	glm::vec3 m_ambient;
	glm::vec3 m_diffuse;
	glm::vec3 m_specular;
	glm::vec3 m_emissive;

	float m_specularPower;
	float m_opacity;

	Texture m_diffuseTexture;			// bound slot 0
	Texture m_alphaTexture;				// bound slot 1
	Texture m_ambientTexture;			// bound slot 2
	Texture m_specularTexture;			// bound slot 3
	Texture m_specularHighlightTexture;	// bound slot 4
	Texture m_normalTexture;				// bound slot 5
	Texture m_displacementTexture;		// bound slot 6
};

//a simple mesh wrapper
class Mesh
{
public:
	Mesh() {}
	~Mesh();

	//will fail if a mesh has already been loaded in to this instance
	bool LoadObj(const char* filename, bool loadTextures = true, bool flipTextureV = false);

	//pass in prim type to allow patches vs triangles etc
	void Draw(unsigned int primitiveType);

	//material access
	inline size_t GetMaterialCount() const { return m_materials.size();  }

	inline Material& GetMaterial(size_t index) { return m_materials[index]; }

private:
	void CalculateTangents(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);

	struct MeshChunk
	{
		unsigned int vao;
		unsigned int vbo;
		unsigned int ibo;
		unsigned int indexCount;

		int materialID;
	};

	std::vector<MeshChunk> m_meshChunks;
	std::vector<Material> m_materials;
};

#endif