#ifndef _FBX_FILE_H_
#define _FBX_FILE_H_

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/epsilon.hpp>
#include <map>
#include <vector>
#include <string>
#include <thread>
#include <mutex>

struct ImportAssistor;

//A complete vertex structure with all the data needed from the FBX file
class FBXVertex
{
public:
	enum VertexAttributeFlags
	{
		ePOSITION = (1 << 0),
		eCOLOUR = (1 << 1),
		eNORMAL = (1 << 2),
		eTANGENT = (1 << 3),
		eBINORMAL = (1 << 4),
		eINDICES = (1 << 5),
		eWEIGHTS = (1 << 6),
		eTEXCOORD1 = (1 << 7),
		eTEXCOORD2 = (1 << 8)
	};

	enum Offsets
	{
		PositionOffset = 0,
		ColourOffset = PositionOffset + sizeof(glm::vec4),
		NormalOffset = ColourOffset + sizeof(glm::vec4),
		TangentOffset = NormalOffset + sizeof(glm::vec4),
		BiNormalOffset = TangentOffset + sizeof(glm::vec4),
		IndicesOffset = BiNormalOffset + sizeof(glm::vec4),
		WeightsOffset = IndicesOffset + sizeof(glm::vec4),
		TexCoord1Offset = WeightsOffset + sizeof(glm::vec4),
		TexCoord2Offset = TexCoord1Offset + sizeof(glm::vec2)
	};

	FBXVertex();
	~FBXVertex();

	glm::vec4 m_position;
	glm::vec4 m_colour;
	glm::vec4 m_normal;
	glm::vec4 m_tangent;
	glm::vec4 m_binormal;
	glm::vec4 m_indices;
	glm::vec4 m_weights;

	glm::vec2 m_texCoord1;
	glm::vec2 m_texCoord2;

	bool operator==(const FBXVertex& rhs) const;
	bool operator<(const FBXVertex& rhs) const;

	//internal use only!
	unsigned int m_index[4];
};

struct FBXTexture
{
	FBXTexture();
	~FBXTexture();

	std::string name;
	std::string path;

	unsigned int handle;

	unsigned char* data;

	int width;
	int height;
	int format;
};

//A simple FBX material that supports 8 texture channels
struct FBXMaterial
{
	enum TextureTypes
	{
		DiffuseTexture = 0,
		AmbientTexture,
		GlowTexture,
		SpecularTexture,
		GlossTexture,
		NormalTexture,
		AlphaTexture,
		DisplacementTexture,

		TextureTypes_Count
	};

	FBXMaterial();
	~FBXMaterial();

	std::string name;

	glm::vec4 ambient;	//RGB + Ambient Factor stored in A
	glm::vec4 diffuse;	//RGBA
	glm::vec4 specular;	//RGB + Shininess/Gloss stored in A
	glm::vec4 emissive;	//RGB + Emissive Factor stored in A

	FBXTexture* textures[TextureTypes_Count];

	glm::vec2 textureOffsets[TextureTypes_Count];	//Texture coordinate offset
	glm::vec2 textureTiling[TextureTypes_Count];	//Texture repeat count

	float textureRotation[TextureTypes_Count];	//Texture rotation around Z (2D rotation)
};

//Simple tree node with local/global transforms and children
//Also has a void* user data that the application can make use of
//for storing anything (i.e. the VAO/VBO/IBO data)
class FBXNode
{
public:
	enum NodeType : unsigned int
	{
		NODE = 0,
		MESH,
		LIGHT,
		CAMERA
	};

	FBXNode();
	virtual ~FBXNode();

	//updates global transform based off parent's global and local
	//then updates children
	virtual void UpdateGlobalTransform();

	NodeType m_nodeType;

	std::string m_name;

	glm::mat4 m_localTransform;
	glm::mat4 m_globalTransform;

	FBXNode* m_parent;

	std::vector<FBXNode*> m_children;

	void* m_userData;
};

//A simple mesh node that contains an array of vertices and indices used
//to represent a triangle mesh.
//Also points to a shared material, and stores a bitfield of vertex attributes
class FBXMeshNode : public FBXNode
{
public:
	FBXMeshNode();
	virtual ~FBXMeshNode();

	unsigned int m_vertexAttributes;

	FBXMaterial* m_material;

	std::vector<FBXVertex> m_meshVertices;
	std::vector<unsigned int> m_meshIndices;
};

//A light node that can represent a point, directional, or spot light
class FBXLightNode : public FBXNode
{
public:
	FBXLightNode();
	virtual ~FBXLightNode();

	enum LightType : unsigned int
	{
		Point = 0,
		Directional,
		Spot
	};

	LightType m_type;

	bool m_on;

	glm::vec4 m_lightColour;	//RGB + Light Intensity stored in A
	glm::vec4 m_attenuation;	//(constant,linear,quadratic,0)

	float m_innerAngle;	//spotlight inner cone angle (if a spotlight)
	float m_outerAngle;	//spotlight outer cone angle (if a spotlight)
};

//A camera node with information to create projection matrix
class FBXCameraNode : public FBXNode
{
public:
	FBXCameraNode();
	virtual ~FBXCameraNode();

	//overridden to update m_viewMatrix automatically
	virtual void UpdateGlobalTransform();

	float m_fieldOfView;	//if 0 then orthographic rather than perspective
	float m_aspectRatio;	//if 0 then ratio based off screen resolution
	float m_near;
	float m_far;

	glm::mat4 m_viewMatrix;	//inverse matrix of node's m_globalTransform
};

//A single frame for a bone in an animation
class FBXKeyFrame
{
public:
	FBXKeyFrame();
	~FBXKeyFrame();

	unsigned int m_key;

	glm::quat m_rotation;

	glm::vec3 m_translation;
	glm::vec3 m_scale;
};

//A collection of frames for a single bone in an animation
class FBXTrack
{
public:
	FBXTrack();
	~FBXTrack();

	unsigned int m_boneIndex;
	unsigned int m_keyframeCount;

	FBXKeyFrame* m_keyframes;
};

//An animation that contains a collection of animated bone tracks
class FBXAnimation
{
public:
	FBXAnimation();
	~FBXAnimation();

	//creates a deep-copy of this animation (caller takes ownership of returned data)
	FBXAnimation* Clone() const;

	unsigned int TotalFrames() const;
	float TotalTime(float fps = 24.f) const;

	std::string m_name;

	unsigned int m_startFrame;
	unsigned int m_endFrame;
	unsigned int m_trackCount;

	FBXTrack* m_tracks;
};

//A hierarchy of bones that can be animated
class FBXSkeleton
{
public:
	FBXSkeleton();
	~FBXSkeleton();

	void Evaluate(const FBXAnimation* animation, float time, bool loop = true, float fps = 24.f);
	void UpdateBones();

	unsigned int m_boneCount;

	FBXNode** m_nodes;

	int* m_parentIndex;

	glm::mat4* m_bones;	//ready for use in skinning! (bind pose combined)
	glm::mat4* m_bindPoses;

	void* m_userData;
};

//An FBX scene representing the contents on an FBX file.
//Stores individual items within maps, with names as the key.
//Also has a pointer to the root of the scene's node tree.
class FBXFile
{
public:
	FBXFile() : m_root(nullptr), m_importAssistor(nullptr) {}
	~FBXFile() { Unload(); }

	enum UNIT_SCALE
	{
		UNITS_MILLIMETER,
		UNITS_CENTIMETER,
		UNITS_DECIMETER,
		UNITS_METER,
		UNITS_KILOMETER,
		UNITS_INCH,
		UNITS_FOOT,
		UNITS_YARD,
		UNITS_MILE
	};

	//must unload a scene before loading a new one over top
	bool Load(const char* filename, UNIT_SCALE scale = FBXFile::UNITS_METER, bool loadTextures = true, bool loadAnimations = true, bool flipTextureY = true);
	bool LoadAnimationsOnly(const char* filename, UNIT_SCALE scale = FBXFile::UNITS_METER);

	void Unload();
	//goes through all loaded textures and creates their GL versions
	void InitialiseOpenGLTextures();

	//the folder path of the FBX file
	//useful for accessing texture locations
	const char* GetPath() const { return m_path.c_str(); }

	//the scene arranged in a tree graph
	FBXNode* GetRoot() const { return m_root; }

	//the ambient light of the scene
	const glm::vec4& GetAmbientLight() const { return m_ambientLight; }

	unsigned int GetMeshCount() const { return m_meshes.size(); }
	unsigned int GetLightCount() const { return m_lights.size(); }
	unsigned int GetCameraCount() const { return m_cameras.size(); }
	unsigned int GetMaterialCount() const { return m_materials.size(); }
	unsigned int GetSkeletonCount() const { return m_skeletons.size(); }
	unsigned int GetAnimationCount() const { return m_animations.size(); }
	unsigned int GetTextureCount() const { return m_textures.size(); }

	FBXMeshNode* GetMeshByName(const char* name);
	FBXLightNode* GetLightByName(const char* name);
	FBXCameraNode* GetCameraByName(const char* name);
	FBXMaterial* GetMaterialByName(const char* name);
	FBXAnimation* GetAnimationByName(const char* name);
	FBXTexture* GetTextureByName(const char* name);

	//these methods are slow as the items are stored in a map
	FBXMeshNode* GetMeshByIndex(unsigned int index) const { return m_meshes[index]; }
	FBXLightNode* GetLightByIndex(unsigned int index);
	FBXCameraNode* GetCameraByIndex(unsigned int index);
	FBXMaterial* GetMaterialByIndex(unsigned int index);
	FBXSkeleton* GetSkeletonByIndex(unsigned int index) { return m_skeletons[index]; }
	FBXAnimation* GetAnimationByIndex(unsigned int index);
	FBXTexture* GetTextureByIndex(unsigned int index);

private:
	void ExtractObject(FBXNode* parent, void* object);
	void ExtractMeshes(void* object, void* aieNode);
	void ExtractLight(FBXLightNode* light, void* object);
	void ExtractCamera(FBXCameraNode* camera, void* object);
	void GatherBones(void* object);
	void ExtractSkeleton(FBXSkeleton* skeleton, void* scene);
	void ExtractAnimation(void* scene);
	void ExtractAnimationTrack(std::vector<int>& tracks, void* layer, void* node, std::vector<void*>& nodes, unsigned int& startFrame, unsigned int& endFrame);

	FBXMaterial* ExtractMaterial(void* mesh, int materialIndex);

	static void OptimiseMesh(FBXMeshNode* mesh);
	static void CalculateTangentsBinormals(std::vector<FBXVertex>& vertices, const std::vector<unsigned int>& indices);

	unsigned int NodeCount(FBXNode* node);

private:
	FBXNode* m_root;

	std::string m_path;

	glm::vec4 m_ambientLight;

	std::vector<FBXMeshNode*> m_meshes;
	std::vector<FBXSkeleton*> m_skeletons;
	std::vector<std::thread*> m_threads;

	std::map<std::string, FBXLightNode*> m_lights;
	std::map<std::string, FBXCameraNode*> m_cameras;
	std::map<std::string, FBXMaterial*> m_materials;
	std::map<std::string, FBXTexture*> m_textures;
	std::map<std::string, FBXAnimation*> m_animations;

	ImportAssistor* m_importAssistor;

	//threads used during loading
	std::mutex m_textureMutex;
	std::mutex m_materialMutex;
	std::mutex m_meshesMutex;
	std::mutex m_testMutex;
};

//////////////////////////////////////////////////////////////////////////
inline FBXVertex::FBXVertex() : m_position(0, 0, 0, 1), m_colour(1, 1, 1, 1), m_normal(0, 0, 0, 0), m_tangent(0, 0, 0, 0), m_binormal(0, 0, 0, 0), m_indices(0, 0, 0, 0), m_weights(0, 0, 0, 0),
								m_texCoord1(0, 0), m_texCoord2(0, 0) {}

inline FBXVertex::~FBXVertex() {}

inline bool FBXVertex::operator==(const FBXVertex& rhs) const { return memcmp(this, &rhs, sizeof(FBXVertex)) == 0; }

inline bool FBXVertex::operator<(const FBXVertex& rhs) const { return memcmp(this, &rhs, sizeof(FBXVertex)) < 0; }

inline FBXTexture::FBXTexture() : data(nullptr), handle(0), width(0), height(0), format(0) {}

inline FBXMaterial::FBXMaterial() : ambient(0, 0, 0, 0), diffuse(1, 1, 1, 1), specular(1, 1, 1, 1), emissive(0, 0, 0, 0)
{
	memset(textures, 0, TextureTypes_Count * sizeof(FBXTexture*));
	memset(textureOffsets, 0, TextureTypes_Count * sizeof(glm::vec2));
	memset(textureTiling, 0, TextureTypes_Count * sizeof(glm::vec2));
	memset(textureRotation, 0, TextureTypes_Count * sizeof(float));
}

inline FBXMaterial::~FBXMaterial() {}

inline FBXNode::FBXNode() : m_nodeType(NODE), m_localTransform(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1), m_globalTransform(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1), 
							m_parent(nullptr), m_userData(nullptr) {}

inline FBXNode::~FBXNode()
{
	for (auto n : m_children)
		delete n;
}

inline FBXMeshNode::FBXMeshNode() : m_vertexAttributes(0), m_material(nullptr) { m_nodeType = MESH; }

inline FBXMeshNode::~FBXMeshNode() {}

inline FBXLightNode::FBXLightNode() { m_nodeType = LIGHT; }

inline FBXLightNode::~FBXLightNode() {}

inline FBXCameraNode::FBXCameraNode() { m_nodeType = CAMERA; }

inline FBXCameraNode::~FBXCameraNode() {}

inline FBXKeyFrame::FBXKeyFrame() : m_key(0), m_rotation(0, 0, 0, 1), m_translation(0, 0, 0), m_scale(1, 1, 1) {}

inline FBXKeyFrame::~FBXKeyFrame() {}

inline FBXTrack::FBXTrack() : m_boneIndex(0), m_keyframeCount(0), m_keyframes(nullptr) {}

inline FBXTrack::~FBXTrack() { delete[] m_keyframes; }

inline FBXAnimation::FBXAnimation() : m_startFrame(0xffffffff), m_endFrame(0), m_trackCount(0), m_tracks(nullptr) {}

inline FBXAnimation::~FBXAnimation() { delete[] m_tracks; }

inline unsigned int FBXAnimation::TotalFrames() const { return m_endFrame - m_startFrame; }

inline float FBXAnimation::TotalTime(float fps /* = 24.f */) const { return (m_endFrame - m_startFrame) / fps; }

inline FBXSkeleton::FBXSkeleton() : m_boneCount(0), m_nodes(nullptr), m_parentIndex(nullptr), m_bones(nullptr), m_bindPoses(nullptr), m_userData(nullptr) {}

inline FBXSkeleton::~FBXSkeleton()
{
	delete[] m_parentIndex;
	delete[] m_nodes;
	delete[] m_bones;
	delete[] m_bindPoses;
}

#endif