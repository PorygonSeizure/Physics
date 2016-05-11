#include "Render.h"

#include <vector>
#include "gl_core_4_4.h"
#include "GLFW/glfw3.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

using namespace tinyobj;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <Shader.h>

struct Texture
{
	std::string path;
	unsigned int glID;
};

struct TextureBank { std::vector<Texture> textures; };

TextureBank textureBank;

//bool LoadShaderType(char* filename, GLenum shaderType, unsigned int* output)
//{
//    //we want to be able to return if we succeded
//    bool succeeded = true;
//
//    //open the shader file
//	FILE* shaderFile;
//	fopen_s(&shaderFile, filename, "r");
//
//    //did it open successfully 
//    if (shaderFile == 0)
//        succeeded = false;
//    else
//    {
//        //find out how long the file is
//        fseek(shaderFile, 0, SEEK_END);
//        int shaderFileLength = ftell(shaderFile);
//        fseek(shaderFile, 0, SEEK_SET);
//        //allocate enough space for the file
//        char* shaderSource = new char[shaderFileLength];
//        //read the file and update the length to be accurate
//        shaderFileLength = fread(shaderSource, 1, shaderFileLength, shaderFile);
//
//        //create the shader based on the type that got passed in
//        unsigned int shaderHandle = glCreateShader(shaderType);
//        //compile the shader
//        glShaderSource(shaderHandle, 1, &shaderSource, &shaderFileLength);
//        glCompileShader(shaderHandle);
//
//        //chech the shader for errors
//        int success = GL_FALSE;
//        glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &success);
//        if (success == GL_FALSE)
//        {
//            int logLength = 0;
//            glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &logLength);
//            char* log = new char[logLength];
//            glGetShaderInfoLog(shaderHandle, logLength, NULL, log);
//            printf("%s\n", log);
//            delete[] log;
//            succeeded = false;
//        }
//        //only give the result to the caller if we succeeded
//        if (succeeded)
//            *output = shaderHandle;
//
//        //clean up the stuff we allocated
//        delete[] shaderSource;
//        fclose(shaderFile);
//    }
//
//    return succeeded;
//}

//bool LoadShader(char* vertexFilename, char* geometryFilename, char* fragmentFilename, GLuint* result)
//{
//    bool succeeded = true;
//
//    *result = glCreateProgram();
//
//    unsigned int vertexShader;
//
//    if (LoadShaderType(vertexFilename, GL_VERTEX_SHADER, &vertexShader))
//    {
//        glAttachShader(*result, vertexShader);
//        glDeleteShader(vertexShader);
//    }
//    else
//        printf("FAILED TO LOAD VERTEX SHADER\n");
//
//    if (geometryFilename != nullptr)
//    {
//        unsigned int geometryShader;
//        if (LoadShaderType(geometryFilename, GL_GEOMETRY_SHADER, &geometryShader))
//        {
//            glAttachShader(*result, geometryShader);
//            glDeleteShader(geometryShader);
//        }
//        else
//            printf("FAILED TO LOAD GEOMETRY SHADER\n");
//    }
//    if (fragmentFilename != nullptr)
//    {
//        unsigned int fragmentShader;
//        if (LoadShaderType(fragmentFilename, GL_FRAGMENT_SHADER, &fragmentShader))
//        {
//            glAttachShader(*result, fragmentShader);
//            glDeleteShader(fragmentShader);
//        }
//        else
//            printf("FAILED TO LOAD FRAGMENT SHADER\n");
//    }
//
//    glLinkProgram(*result);
//
//    GLint success;
//    glGetProgramiv(*result, GL_LINK_STATUS, &success);
//    if (success == GL_FALSE)
//    {
//        GLint logLength;
//        glGetProgramiv(*result, GL_INFO_LOG_LENGTH, &logLength);
//        char* log = new char[logLength];
//        glGetProgramInfoLog(*result, logLength, 0, log);
//
//        printf("ERROR: STUFF DONE SCREWED UP IN UR SHADER BUDDY!\n\n");
//        printf("%s", log);
//
//        delete[] log;
//        succeeded = false;
//    }
//
//    return succeeded;
//}

unsigned int CreateGLTextureBasic(unsigned char* data, int width, int height, int channels)
{
    GLenum format = 0;
    GLenum srcFormat = 0;
    switch (channels)
    {
    case 1:
    {
        format = GL_R8;
        srcFormat = GL_RED;
		break;
    }
    case 2:
    {
        format = GL_RG8;
        srcFormat = GL_RG;
		break;
    }
    case 3:
    {
        format = GL_RGB8;
        srcFormat = GL_RGB;
		break;
    }
    case 4:
    {
        format = GL_RGBA8;
        srcFormat = GL_RGBA;
		break;
    }
    }

	unsigned int texHandle;
	glGenTextures(1, (GLuint*)&texHandle);
	glBindTexture(GL_TEXTURE_2D, texHandle);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, srcFormat, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	return texHandle;
}

unsigned LoadGLTextureBasic(const char* filename)
{
    unsigned int result = 0;

	if (filename)
	{
		for (unsigned int i = 0; i < textureBank.textures.size(); ++i)
		{
			if (textureBank.textures[i].path == filename)
				result = textureBank.textures[i].glID;
		}

		if (result == 0)
		{
			int width;
			int height;
			int channels;
			unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);
			if (data)
			{
				result = CreateGLTextureBasic(data, width, height, channels);
				stbi_image_free(data);

				Texture tex;
				tex.glID = result;
				tex.path = filename;
				textureBank.textures.push_back(tex);
			}
		}
	}

    return result;
}

Mesh* CreateMeshFromBuffers(SimpleVertex* vertexData, unsigned int vertexCount, unsigned int* indexData, unsigned int indexCount, Material material)
{
	if (!vertexData || !indexData)
		return 0;

	//NOTE(aidan): all data for the scene amortized into a single buffer for easy deallocation
	//			This is easy to do as its all just simple POD types
	size_t totalBytes = sizeof(Mesh) + sizeof(SubMesh) + sizeof(SimpleVertex) * vertexCount + sizeof(unsigned int) * indexCount;
	void* memory = calloc(totalBytes, 1);

	Mesh* result = (Mesh*)memory;
	SubMesh* subMesh = (SubMesh*)(result + 1);
	SimpleVertex* verts = (SimpleVertex*)(subMesh + 1);
	unsigned int* indices = (unsigned int*)(verts + vertexCount);

	memcpy(verts, vertexData, sizeof(SimpleVertex) * vertexCount);
	memcpy(indices, indexData, sizeof(unsigned int) * indexCount);

	result->subMeshes = subMesh;
	result->subMeshCount = 1;
	result->vertexCount = vertexCount;
	result->vertexData = verts;

	subMesh->indexCount = indexCount;
	subMesh->indexData = indices;
	subMesh->material = material;

	glGenBuffers(1, &result->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, result->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(SimpleVertex) * result->vertexCount, result->vertexData, GL_DYNAMIC_DRAW);

	glGenVertexArrays(1, &subMesh->vao);
	glBindVertexArray(subMesh->vao);

	glGenBuffers(1, &subMesh->ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subMesh->ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * subMesh->indexCount, subMesh->indexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)(sizeof(float) * 3));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)(sizeof(float) * 6));

	glBindVertexArray(0);

	return result;
}

void FreeMesh(Mesh* mesh)
{
	*mesh = {};
	free(mesh);
}

void RebuildVertexBuffer(Mesh* mesh)
{
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, mesh->vertexCount * sizeof(SimpleVertex), mesh->vertexData);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


Renderer::Renderer()
{
	mainShader = new Shader();
	//LoadShader("shaders/mainShader.vs", 0, "shaders/mainShader.fs", &mainShader);
	mainShader->LoadShader(GL_VERTEX_SHADER, "./res/shaders/mainShader.vs");
	mainShader->LoadShader(GL_FRAGMENT_SHADER, "./res/shaders/mainShader.fs");
	mainShader->Link();
}

Renderer::~Renderer() {}

void Renderer::PushMesh(Mesh* mesh, mat4 transform)
{
    RenderItem item = {};
    item.mesh = mesh;
    item.transform = transform;

    renderQueue.push_back(item);
}

void Renderer::RenderAndClear(mat4 viewProj)
{
    //glUseProgram(mainShader);
	mainShader->Bind();

    //int viewProjLoc = glGetUniformLocation(mainShader, "viewProj");
	//
    //int modelLoc = glGetUniformLocation(mainShader, "model");
    //int modelViewProjLoc = glGetUniformLocation(mainShader, "modelViewProj");
	//
    //int diffuseLoc = glGetUniformLocation(mainShader, "diffuseTex");
    //int normalLoc = glGetUniformLocation(mainShader, "normalTex");
    //int specularLoc = glGetUniformLocation(mainShader, "specularTex");
	//
    //int ambientLocation = glGetUniformLocation(mainShader, "ambientLight");
    //int lightDirLocation = glGetUniformLocation(mainShader, "lightDir");
    //int lightColorLocation = glGetUniformLocation(mainShader, "lightColor");
    //int specPowLocation = glGetUniformLocation(mainShader, "specularPower");

	int viewProjLoc = mainShader->GetUniform("viewProj");

	int modelLoc = mainShader->GetUniform("model");
	int modelViewProjLoc = mainShader->GetUniform("modelViewProj");

	int diffuseLoc = mainShader->GetUniform("diffuseTex");
	int normalLoc = mainShader->GetUniform("normalTex");
	int specularLoc = mainShader->GetUniform("specularTex");

	int ambientLocation = mainShader->GetUniform("ambientLight");
	int lightDirLocation = mainShader->GetUniform("lightDir");
	int lightColorLocation = mainShader->GetUniform("lightColor");
	int specPowLocation = mainShader->GetUniform("specularPower");

    float sq3 = sqrt(3.f);

    glUniform3f(ambientLocation, 0.2f, 0.2f, 0.2f);
    glUniform3f(lightDirLocation, sq3, -sq3, sq3);
    glUniform3f(lightColorLocation, 0.8f, 0.8f, 0.8f);
    glUniform1f(specPowLocation, 15.f);

    glUniformMatrix4fv(viewProjLoc, 1, GL_FALSE, (float*)&viewProj);
    glUniform1i(diffuseLoc, 0);
    glUniform1i(normalLoc, 1);
    glUniform1i(specularLoc, 2);

	for (unsigned int i = 0; i < renderQueue.size(); ++i)
	{
		RenderItem* item = &renderQueue[i];

		mat4 modelViewProj = viewProj * item->transform;

		Mesh* mesh = item->mesh;

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, (float*)&item->transform);
		glUniformMatrix4fv(modelViewProjLoc, 1, GL_FALSE, (float*)&modelViewProj);

		for (unsigned int j = 0; j < mesh->subMeshCount; ++j)
		{
			SubMesh* subMesh = mesh->subMeshes + j;

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, subMesh->material.diffuseTexture);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, subMesh->material.normalTexture);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, subMesh->material.specularTexture);

			glBindVertexArray(subMesh->vao);
			glDrawElements(GL_TRIANGLES, subMesh->indexCount, GL_UNSIGNED_INT, 0);
		}
    }

    glBindVertexArray(0);

	renderQueue.clear();
}

Scene LoadSceneFromOBJ(char* dir, char* filename)
{
	Scene result = {};

	if (filename && dir)
	{
		std::string filePath = std::string(dir) + std::string(filename);

		//load OBJ
		std::vector<shape_t> shapes;
		std::vector<material_t> materials;
		std::string err;
		LoadObj(shapes, materials, err, filePath.c_str(), dir);

		std::string dirStr = "";
		if (dir)
			dirStr = dir;

		//compute memory requirements
		int totalVertexCount = 0;
		int totalIndexCount = 0;
		int totalMeshCount = shapes.size();
		int totalSubMeshCount = 0;

		for (unsigned int shape = 0; shape < shapes.size(); ++shape)
		{
			totalVertexCount += shapes[shape].mesh.positions.size() / 3;
			totalIndexCount += shapes[shape].mesh.material_ids.size() * 3;
		}

		//need to know about one more than existing materials
		//as meshes can have be assigned to a 'null' material
		int mp1 = materials.size() + 1;

		int* materialsUsed = new int[mp1 * shapes.size()];
		memset(materialsUsed, 0, sizeof(int) * mp1 * shapes.size());

		for (unsigned int meshIndex = 0; meshIndex < shapes.size(); ++meshIndex)
		{
			mesh_t* mesh = &shapes[meshIndex].mesh;

			for (unsigned int j = 0; j < mesh->material_ids.size(); ++j)
			{
				int index = mesh->material_ids[j]+1;
				materialsUsed[mp1 * meshIndex + index]++;
			}

			for (int j = 0; j < mp1; ++j)
				if (materialsUsed[mp1 * meshIndex + j] != 0)
					++totalSubMeshCount;
		}
		
		Material defaultMaterial = {};
		defaultMaterial.diffuseColor = vec3(255, 105, 180);
		defaultMaterial.specularColor = vec3(255, 105, 180);

		//All memory for the scene amortized into a single buffer for easy dealocation
		unsigned int totalBytes = totalMeshCount * sizeof(Mesh) + totalSubMeshCount * sizeof(SubMesh) + totalVertexCount * sizeof(SimpleVertex) + totalIndexCount * sizeof(unsigned int);
		unsigned char* buffer = (unsigned char*)calloc(totalBytes, 1);

		//sub-allocate out the buffer
		Mesh* meshBuffer = (Mesh*)buffer;
		SubMesh* subMeshBuffer = (SubMesh*)(meshBuffer + totalMeshCount);
		SimpleVertex* vertexBuffer = (SimpleVertex*)(subMeshBuffer + totalSubMeshCount);
		unsigned int* indexBuffer = (unsigned int*)(vertexBuffer + totalVertexCount);

		result.meshes = meshBuffer;
		result.meshCount = totalMeshCount;

		//loop over meshes to partition out data to the internal format
		for (unsigned int shapeIndex = 0; shapeIndex < shapes.size(); ++shapeIndex)
		{
			Mesh* mesh = meshBuffer;
			mesh_t* objMesh = &shapes[shapeIndex].mesh;

			mesh->vertexCount = objMesh->positions.size() / 3;
			mesh->vertexData = vertexBuffer;

			if (objMesh->positions.size() == mesh->vertexCount * 3)
			{
				for (unsigned int vert = 0; vert < mesh->vertexCount; ++vert)
				{
					mesh->vertexData[vert].pos.x = objMesh->positions[vert * 3 + 0] / 100.f;
					mesh->vertexData[vert].pos.y = objMesh->positions[vert * 3 + 1] / 100.f;
					mesh->vertexData[vert].pos.z = objMesh->positions[vert * 3 + 2] / 100.f;
				}
			}
			
			if (objMesh->texcoords.size() == mesh->vertexCount * 2)
			{
				for (unsigned int vert = 0; vert < mesh->vertexCount; ++vert)
				{
					mesh->vertexData[vert].uv.x = objMesh->texcoords[vert * 2 + 0];
					mesh->vertexData[vert].uv.y = objMesh->texcoords[vert * 2 + 1];
				}
			}

			if (objMesh->normals.size() == mesh->vertexCount * 3)
			{
				for (unsigned int vert = 0; vert < mesh->vertexCount; ++vert)
				{
					mesh->vertexData[vert].normal.x = objMesh->normals[vert * 3 + 0];
					mesh->vertexData[vert].normal.y = objMesh->normals[vert * 3 + 1];
					mesh->vertexData[vert].normal.z = objMesh->normals[vert * 3 + 2];
				}
			}

			mesh->subMeshes = subMeshBuffer;

			//maps material index to submesh pointer
			SubMesh** subMeshMap = new SubMesh*[mp1];
			memset(subMeshMap, 0, sizeof(SubMesh*) * mp1);

			for (int j = 0; j < mp1; ++j)
			{
				if (materialsUsed[mp1 * shapeIndex + j] != 0)
				{
					subMeshMap[j] = subMeshBuffer + mesh->subMeshCount;
					subMeshMap[j]->indexCount = materialsUsed[mp1 * shapeIndex + j] * 3;

					//assign materials
					if (j == 0)
						subMeshMap[j]->material = defaultMaterial;
					else
					{
						memcpy(&subMeshMap[j]->material.diffuseColor, materials[j - 1].diffuse, sizeof(float) * 3);
						memcpy(&subMeshMap[j]->material.specularColor, materials[j - 1].specular, sizeof(float) * 3);

						std::string diffuseFilename = (dirStr + materials[j - 1].diffuse_texname);
						std::string bumpFilename = (dirStr + materials[j - 1].bump_texname);
						std::string specularFilename = (dirStr + materials[j - 1].specular_texname);

						subMeshMap[j]->material.diffuseTexture = LoadGLTextureBasic(diffuseFilename.c_str());
						subMeshMap[j]->material.normalTexture = LoadGLTextureBasic(bumpFilename.c_str());
						subMeshMap[j]->material.specularTexture = LoadGLTextureBasic(specularFilename.c_str());
					}
					++mesh->subMeshCount;
				}
			}

			//sub-allocate index buffers
			for (unsigned int j = 0; j < mesh->subMeshCount; ++j)
			{
				mesh->subMeshes[j].indexData = indexBuffer;
				indexBuffer += mesh->subMeshes[j].indexCount;
			}
			
			int* indexCounts = new int[mesh->subMeshCount];
			memset(indexCounts, 0, sizeof(int) * mesh->subMeshCount);

			for (unsigned int j = 0; j < objMesh->material_ids.size(); ++j)
			{
				int id = objMesh->material_ids[j];
				SubMesh* subMesh = subMeshMap[id+1];

				int subMeshIndex = (int)(subMesh - mesh->subMeshes);

				int index = indexCounts[subMeshIndex];

				subMesh->indexData[index + 0] = objMesh->indices[j * 3 + 0];
				subMesh->indexData[index + 1] = objMesh->indices[j * 3 + 1];
				subMesh->indexData[index + 2] = objMesh->indices[j * 3 + 2];

				indexCounts[subMeshIndex] += 3;
			}

			glGenBuffers(1, &mesh->vbo);
			glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(SimpleVertex) * mesh->vertexCount, mesh->vertexData, GL_STATIC_DRAW);
			
			for (unsigned int j = 0; j < mesh->subMeshCount; ++j)
			{
				SubMesh* subMesh = mesh->subMeshes + j;

				glGenVertexArrays(1, &subMesh->vao);
				glBindVertexArray(subMesh->vao);

				glGenBuffers(1, &subMesh->ibo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subMesh->ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * subMesh->indexCount, subMesh->indexData, GL_STATIC_DRAW);

				glEnableVertexAttribArray(0);
				glEnableVertexAttribArray(1);
				glEnableVertexAttribArray(2);

				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), 0);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)(sizeof(float) * 3));
				glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)(sizeof(float) * 6));

				glBindVertexArray(0);
			}

			delete[] indexCounts;
			delete[] subMeshMap;

			subMeshBuffer += mesh->subMeshCount;
			meshBuffer++;
			vertexBuffer += mesh->vertexCount;
		}

		delete[] materialsUsed;
	}

	return result;
}

void FreeSceneFromOBJ(Scene* scene)
{
	free(scene->meshes);
	*scene = {};
}