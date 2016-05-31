#define STB_IMAGE_IMPLEMENTATION
#include "Mesh.h"
#include "gl_core_4_4.h"
#include <glm/geometric.hpp>
#include <stb_image.h>
#include "tinyOBJLoader.h"

using std::vector;
using std::string;
using glm::vec3;
using glm::vec4;
using glm::vec2;

Texture::~Texture() { glDeleteTextures(1, &m_handle); }

Mesh::~Mesh()
{
	for (auto& c : m_meshChunks)
	{
		glDeleteVertexArrays(1, &c.vao);
		glDeleteBuffers(1, &c.vbo);
		glDeleteBuffers(1, &c.ibo);
	}
}

bool Mesh::LoadObj(const char* filename, bool loadTextures /*= true*/, bool flipTextureV /*= false*/)
{
	if (!m_meshChunks.empty())
	{
		printf("Mesh already initialised, can't re-initialise!\n");
		return false;
	}

	vector<tinyobj::ShapeT> shapes;
	vector<tinyobj::MaterialT> materials;
	string error = "";

	string file = filename;
	string folder = file.substr(0, file.find_last_of('/') + 1);

	bool success = tinyobj::LoadObj(shapes, materials, error, filename, folder.c_str());

	if (!success)
	{
		printf("%s\n", error.c_str());
		return false;
	}

	//copy materials
	m_materials.resize(materials.size());
	int index = 0;
	for (auto& m : materials)
	{
		m_materials[index].m_ambient = vec3(m.ambient[0], m.ambient[1], m.ambient[2]);
		m_materials[index].m_diffuse = vec3(m.diffuse[0], m.diffuse[1], m.diffuse[2]);
		m_materials[index].m_specular = vec3(m.specular[0], m.specular[1], m.specular[2]);
		m_materials[index].m_emissive = vec3(m.emission[0], m.emission[1], m.emission[2]);
		m_materials[index].m_specularPower = m.shininess;
		m_materials[index].m_opacity = m.dissolve;

		//textures
		m_materials[index].m_alphaTexture.LoadTexture((folder + m.alphaTexname).c_str());
		m_materials[index].m_ambientTexture.LoadTexture((folder + m.ambientTexname).c_str());
		m_materials[index].m_diffuseTexture.LoadTexture((folder + m.diffuseTexname).c_str());
		m_materials[index].m_specularTexture.LoadTexture((folder + m.specularTexname).c_str());
		m_materials[index].m_specularHighlightTexture.LoadTexture((folder + m.specularHighlightTexname).c_str());
		m_materials[index].m_normalTexture.LoadTexture((folder + m.bumpTexname).c_str());
		m_materials[index].m_displacementTexture.LoadTexture((folder + m.displacementTexname).c_str());

		++index;
	}

	//copy shapes
	m_meshChunks.reserve(shapes.size());
	for (auto& s : shapes)
	{
		MeshChunk chunk;

		//generate buffers
		glGenBuffers(1, &chunk.vbo);
		glGenBuffers(1, &chunk.ibo);
		glGenVertexArrays(1, &chunk.vao);

		//bind vertex array aka a mesh wrapper
		glBindVertexArray(chunk.vao);

		//set the index buffer data
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk.ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, s.mesh.indices.size() * sizeof(unsigned int), s.mesh.indices.data(), GL_STATIC_DRAW);

		//store index count for rendering
		chunk.indexCount = s.mesh.indices.size();

		//create vertex data
		vector<Vertex> vertices;
		vertices.resize(s.mesh.positions.size() / 3);
		size_t vertCount = vertices.size();

		bool hasPosition = !s.mesh.positions.empty();
		bool hasNormal = !s.mesh.normals.empty();
		bool hasTexture = !s.mesh.texcoords.empty();

		for (size_t i = 0; i < vertCount; ++i)
		{
			if (hasPosition)
				vertices[i].position = vec4(s.mesh.positions[i * 3 + 0], s.mesh.positions[i * 3 + 1], s.mesh.positions[i * 3 + 2], 1);
			if (hasNormal)
				vertices[i].normal = vec4(s.mesh.normals[i * 3 + 0], s.mesh.normals[i * 3 + 1], s.mesh.normals[i * 3 + 2], 0);

			//flip the T / V (might not always be needed, depends on how mesh was made)
			if (hasTexture)
				vertices[i].texcoord = vec2(s.mesh.texcoords[i * 2 + 0], flipTextureV ? 1.0f - s.mesh.texcoords[i * 2 + 1] : s.mesh.texcoords[i * 2 + 1]);

			vertices[i].colour = vec4(1, 0, 0, 1);
		}

		//calculate for normal mapping
		if (hasNormal && hasTexture)
			CalculateTangents(vertices, s.mesh.indices);

		//bind vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo);

		//fill vertex buffer
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

		//enable first element as positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);

		//enable colours
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(vec4)));

		//enable normals
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_TRUE, sizeof(Vertex), (void*)(sizeof(vec4) * 2));

		//enable tangent
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(vec4) * 3));

		//enable tex coords
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 2, GL_FLOAT, GL_TRUE, sizeof(Vertex), (void*)(sizeof(vec4) * 4));

		//bind 0 for safety
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		//set chunk material
		chunk.materialID = s.mesh.materialIDs.empty() ? -1 : s.mesh.materialIDs[0];

		m_meshChunks.push_back(chunk);
	}

	//load obj
	return true;
}

void Mesh::Draw(unsigned int primitiveType)
{
	int program = -1;
	glGetIntegerv(GL_CURRENT_PROGRAM, &program);

	if (program == -1)
	{
		printf("No shader bound!\n");
		return;
	}

	//pull uniforms from the shader
	int kaUniform = glGetUniformLocation(program, "lightAmbient");
	int kdUniform = glGetUniformLocation(program, "lightDiffuse");
	int ksUniform = glGetUniformLocation(program, "lightSpecular");
	int keUniform = glGetUniformLocation(program, "lightEmissive");
	int opacityUniform = glGetUniformLocation(program, "opacity");
	int specPowUniform = glGetUniformLocation(program, "specularPower");

	int alphaTexUniform = glGetUniformLocation(program, "alphaTexture");
	int ambientTexUniform = glGetUniformLocation(program, "ambientTexture");
	int diffuseTexUniform = glGetUniformLocation(program, "diffuseTexture");
	int specTexUniform = glGetUniformLocation(program, "specularTexture");
	int specHighlightTexUniform = glGetUniformLocation(program, "specularHighlightTexture");
	int normalTexUniform = glGetUniformLocation(program, "normalTexture");
	int dispTexUniform = glGetUniformLocation(program, "displacementTexture");

	//set texture slots (these don't change per material)
	if (diffuseTexUniform >= 0)
		glUniform1i(diffuseTexUniform, 0);
	if (alphaTexUniform >= 0)
		glUniform1i(alphaTexUniform, 1);
	if (ambientTexUniform >= 0)
		glUniform1i(ambientTexUniform, 2);
	if (specTexUniform >= 0)
		glUniform1i(specTexUniform, 3);
	if (specHighlightTexUniform >= 0)
		glUniform1i(specHighlightTexUniform, 4);
	if (normalTexUniform >= 0)
		glUniform1i(normalTexUniform, 5);
	if (dispTexUniform >= 0)
		glUniform1i(dispTexUniform, 6);

	int currentMaterial = -1;

	//draw the mesh chunks
	for (auto& c : m_meshChunks)
	{
		//bind material
		if (currentMaterial != c.materialID)
		{
			currentMaterial = c.materialID;
			if (kaUniform >= 0)
				glUniform3fv(kaUniform, 1, &m_materials[currentMaterial].m_ambient[0]);
			if (kdUniform >= 0)
				glUniform3fv(kdUniform, 1, &m_materials[currentMaterial].m_diffuse[0]);
			if (ksUniform >= 0)
				glUniform3fv(ksUniform, 1, &m_materials[currentMaterial].m_specular[0]);
			if (keUniform >= 0)
				glUniform3fv(keUniform, 1, &m_materials[currentMaterial].m_emissive[0]);
			if (opacityUniform >= 0)
				glUniform1f(opacityUniform, m_materials[currentMaterial].m_opacity);
			if (specPowUniform >= 0)
				glUniform1f(specPowUniform, m_materials[currentMaterial].m_specularPower);

			glActiveTexture(GL_TEXTURE0);
			if (m_materials[currentMaterial].m_diffuseTexture.m_handle > 0)
				glBindTexture(GL_TEXTURE_2D, m_materials[currentMaterial].m_diffuseTexture.m_handle);
			else if (diffuseTexUniform >= 0)
				glBindTexture(GL_TEXTURE_2D, 0);

			glActiveTexture(GL_TEXTURE1);
			if (m_materials[currentMaterial].m_alphaTexture.m_handle > 0)
				glBindTexture(GL_TEXTURE_2D, m_materials[currentMaterial].m_alphaTexture.m_handle);
			else if (alphaTexUniform >= 0)
				glBindTexture(GL_TEXTURE_2D, 0);

			glActiveTexture(GL_TEXTURE2);
			if (m_materials[currentMaterial].m_ambientTexture.m_handle > 0)
				glBindTexture(GL_TEXTURE_2D, m_materials[currentMaterial].m_ambientTexture.m_handle);
			else if (ambientTexUniform >= 0)
				glBindTexture(GL_TEXTURE_2D, 0);

			glActiveTexture(GL_TEXTURE3);
			if (m_materials[currentMaterial].m_specularTexture.m_handle > 0)
				glBindTexture(GL_TEXTURE_2D, m_materials[currentMaterial].m_specularTexture.m_handle);
			else if (specTexUniform >= 0)
				glBindTexture(GL_TEXTURE_2D, 0);

			glActiveTexture(GL_TEXTURE4);
			if (m_materials[currentMaterial].m_specularHighlightTexture.m_handle > 0)
				glBindTexture(GL_TEXTURE_2D, m_materials[currentMaterial].m_specularHighlightTexture.m_handle);
			else if (specHighlightTexUniform >= 0)
				glBindTexture(GL_TEXTURE_2D, 0);

			glActiveTexture(GL_TEXTURE5);
			if (m_materials[currentMaterial].m_normalTexture.m_handle > 0)
				glBindTexture(GL_TEXTURE_2D, m_materials[currentMaterial].m_normalTexture.m_handle);
			else if (normalTexUniform >= 0)
				glBindTexture(GL_TEXTURE_2D, 0);

			glActiveTexture(GL_TEXTURE6);
			if (m_materials[currentMaterial].m_displacementTexture.m_handle > 0)
				glBindTexture(GL_TEXTURE_2D, m_materials[currentMaterial].m_displacementTexture.m_handle);
			else if (dispTexUniform >= 0)
				glBindTexture(GL_TEXTURE_2D, 0);
		}

		//bind and draw geometry
		glBindVertexArray(c.vao);
		glDrawElements(primitiveType, c.indexCount, GL_UNSIGNED_INT, 0);
	}
}

bool Texture::LoadTexture(const char* path)
{
	int x = 0;
	int y = 0;
	int comp = 0;
	unsigned char* data = stbi_load(path, &x, &y, &comp, STBI_default);

	if (data)
	{
		glGenTextures(1, &m_handle);
		glBindTexture(GL_TEXTURE_2D, m_handle);
		switch (comp)
		{
		case STBI_grey:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, x, y, 0, GL_RED, GL_UNSIGNED_BYTE, data);
			m_format = GL_RED;
			break;
		case STBI_grey_alpha:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, x, y, 0, GL_RG, GL_UNSIGNED_BYTE, data);
			m_format = GL_RG;
			break;
		case STBI_rgb:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			m_format = GL_RGB;
			break;
		case STBI_rgb_alpha:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			m_format = GL_RGBA;
			break;
		default:
			break;
		};
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		stbi_image_free(data);
		m_width = x;
		m_height = y;
		m_filename = path;
		return true;
	}

	return false;
}

void Mesh::CalculateTangents(vector<Vertex>& vertices, const vector<unsigned int>& indices)
{
	unsigned int vertexCount = vertices.size();
	vec4* tan1 = new vec4[vertexCount * 2];
	vec4* tan2 = tan1 + vertexCount;
	memset(tan1, 0, vertexCount * sizeof(vec4) * 2);

	unsigned int indexCount = indices.size();
	for (unsigned int a = 0; a < indexCount; a += 3)
	{
		long i1 = indices[a];
		long i2 = indices[a + 1];
		long i3 = indices[a + 2];

		const vec4& v1 = vertices[i1].position;
		const vec4& v2 = vertices[i2].position;
		const vec4& v3 = vertices[i3].position;

		const vec2& w1 = vertices[i1].texcoord;
		const vec2& w2 = vertices[i2].texcoord;
		const vec2& w3 = vertices[i3].texcoord;

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.x - w1.x;
		float s2 = w3.x - w1.x;
		float t1 = w2.y - w1.y;
		float t2 = w3.y - w1.y;

		float r = 1.f / (s1 * t2 - s2 * t1);
		vec4 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r, 0);
		vec4 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r, 0);

		tan1[i1] += sdir;
		tan1[i2] += sdir;
		tan1[i3] += sdir;

		tan2[i1] += tdir;
		tan2[i2] += tdir;
		tan2[i3] += tdir;
	}

	for (unsigned int a = 0; a < vertexCount; a++)
	{
		const vec3& n = vec3(vertices[a].normal);
		const vec3& t = vec3(tan1[a]);

		//Gram-Schmidt orthogonalize
		vertices[a].tangent = vec4(normalize(t - n * dot(n, t)), 0);

		//Calculate handedness (direction of bitangent)
		vertices[a].tangent.w = (dot(cross(vec3(n), vec3(t)), vec3(tan2[a])) < 0.f) ? 1.f : -1.f;

		//calculate bitangent (ignoring for Vertex)
		//vertices[a].bitangent = vec4(cross(vec3(vertices[a].normal), vec3(vertices[a].tangent)) * vertices[a].tangent.w, 0);

		vertices[a].tangent.w = 0;
	}

	delete[] tan1;
}