#include "FBXFile.h"
#include <fbxsdk.h>
#include <algorithm>
#include <set>

//only needed for texture cleanup
#define GLEW_NO_GLU
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/norm.hpp>

using std::vector;
using glm::mat4;
using glm::pi;
using glm::vec4;
using glm::vec3;
using glm::max;
using glm::vec2;

struct ImportAssistor
{
	ImportAssistor() : evaluator(nullptr), loadAnimationOnly(false) {}
	~ImportAssistor() { evaluator = nullptr; }

	FbxScene* scene;

	FbxAnimEvaluator* evaluator;

	FbxImporter* importer;

	vector<FBXNode*> bones;

	bool loadTextures;
	bool loadAnimations;
	bool loadAnimationOnly;
	bool flipTextureY;

	float unitScale;

	std::map<std::string, int> boneIndexList;
};

void FBXFile::Unload()
{
	delete m_root;
	m_root = nullptr;

	for (auto m : m_materials)
		delete m.second;
	for (auto s : m_skeletons)
		delete s;
	for (auto a : m_animations)
		delete a.second;
	for (auto t : m_textures)
		delete t.second;

	m_meshes.clear();
	m_lights.clear();
	m_cameras.clear();
	m_materials.clear();
	m_skeletons.clear();
	m_animations.clear();
	m_textures.clear();
}

bool FBXFile::Load(const char* filename, UNIT_SCALE scale /* = FBXFile::UNITS_METER */, bool loadTextures /* = true */, bool loadAnimations /* = true */, bool flipTextureY /*= true*/)
{
	if (m_root)
	{
		printf("Scene already loaded!\n");
		return false;
	}

	FbxManager* sdkManager = nullptr;
	FbxScene* scene = nullptr;

	//The first thing to do is to create the FBX SDK manager which is the 
	//object allocator for almost all the classes in the SDK.
	sdkManager = FbxManager::Create();
	if (!sdkManager)
	{
		printf("Unable to create the FBX SDK manager\n");
		return false;
	}

	//create an IOSettings object
	FbxIOSettings * ios = FbxIOSettings::Create(sdkManager, IOSROOT);
	sdkManager->SetIOSettings(ios);

	//Create an importer.
	FbxImporter* importer = FbxImporter::Create(sdkManager, "");

	//Initialize the importer by providing a filename.
	bool importStatus = importer->Initialize(filename, -1, sdkManager->GetIOSettings());

	if (!importStatus)
	{
		printf("Call to FbxImporter::Initialize() failed:\n\t%s\n", importer->GetStatus().GetErrorString());
		importer->Destroy();
		return false;
	}

	//Create the entity that will hold the scene.
	int fileMajor;
	int fileMinor;
	int fileRevision;
	int SDKMajor;
	int SDKMinor;
	int SDKRevision;

	unsigned int i;
	bool status;

	//Get the file version number generate by the FBX SDK.
	FbxManager::GetFileFormatVersion(SDKMajor, SDKMinor, SDKRevision);
	importer->GetFileVersion(fileMajor, fileMinor, fileRevision);

	scene = FbxScene::Create(sdkManager, "root");

	//Import the scene.
	status = importer->Import(scene);
	importer->Destroy();
	if (!status)
	{
		printf("Unable to open FBX file!\n");
		return false;
	}

	float unitScale = 1;

	//convert scale
	if (scene->GetGlobalSettings().GetSystemUnit() != FbxSystemUnit::sPredefinedUnits[scale])
	{
		const FbxSystemUnit::ConversionOptions lConversionOptions = {
			false, //mConvertRrsNodes
			true, //mConvertAllLimits
			true, //mConvertClusters
			true, //mConvertLightIntensity
			true, //mConvertPhotometricLProperties
			true  //mConvertCameraClipPlanes
		};

		unitScale = (float)(scene->GetGlobalSettings().GetSystemUnit().GetScaleFactor() / FbxSystemUnit::sPredefinedUnits[scale].GetScaleFactor());

		//Convert the scene to meters using the defined options.
		FbxSystemUnit::sPredefinedUnits[scale].ConvertScene(scene, lConversionOptions);
	}

	//convert the scene to OpenGL axis (right-handed Y up)
	FbxAxisSystem::OpenGL.ConvertScene(scene);

	//DID NOT KNOW WE COULD DO THIS!!!!
	/*
	//Convert mesh, NURBS and patch into triangle mesh
	FbxGeometryConverter lGeomConverter(mSdkManager);
	lGeomConverter.Triangulate(mScene, true);
	//Split meshes per material, so that we only have one material per mesh (for VBO support)
	lGeomConverter.SplitMeshesPerMaterial(mScene, true);
	*/

	FbxNode* node = scene->GetRootNode();

	if (node)
	{
		//store the folder path of the scene
		m_path = filename;
		long lastForward = m_path.find_last_of('/');
		long lastBackward = m_path.find_last_of('\\');
		if (lastForward > lastBackward)
			m_path.resize(lastForward + 1);
		else if (lastBackward)
			m_path.resize(lastBackward + 1);
		else
			m_path = "";

		m_importAssistor = new ImportAssistor();
		m_importAssistor->scene = scene;
		m_importAssistor->evaluator = scene->GetAnimationEvaluator();
		m_importAssistor->importer = importer;
		m_importAssistor->loadTextures = loadTextures;
		m_importAssistor->loadAnimations = loadAnimations;
		m_importAssistor->unitScale = unitScale;
		m_importAssistor->flipTextureY = flipTextureY;

		m_root = new FBXNode();
		m_root->m_name = "root";
		m_root->m_globalTransform = m_root->m_localTransform = mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

		//grab the ambient light data from the scene
		m_ambientLight.x = (float)scene->GetGlobalSettings().GetAmbientColor().mRed;
		m_ambientLight.y = (float)scene->GetGlobalSettings().GetAmbientColor().mGreen;
		m_ambientLight.z = (float)scene->GetGlobalSettings().GetAmbientColor().mBlue;
		m_ambientLight.w = (float)scene->GetGlobalSettings().GetAmbientColor().mAlpha;

		//gather bones to create indices for them in a skeleton
		if (loadAnimations)
		{
			for (i = 0; i < (unsigned int)node->GetChildCount(); ++i)
				GatherBones((void*)node->GetChild(i));
		}

		//extract scene (meshes, lights, cameras)
		for (i = 0; i < (unsigned int)node->GetChildCount(); ++i)
			ExtractObject(m_root, (void*)node->GetChild(i));

		//ensure all threads are finished
		for (auto t : m_threads)
			t->join();
		m_threads.clear();

		//build skeleton and extract animation keyframes
		if (loadAnimations && m_importAssistor->bones.size() > 0)
		{
			FBXSkeleton* skeleton = new FBXSkeleton();
			skeleton->m_boneCount = (unsigned int)m_importAssistor->bones.size();
			skeleton->m_nodes = new FBXNode *[skeleton->m_boneCount];
			skeleton->m_bones = new mat4[skeleton->m_boneCount];
			skeleton->m_bindPoses = new mat4[skeleton->m_boneCount];

			skeleton->m_parentIndex = new int[skeleton->m_boneCount];

			for (i = 0; i < skeleton->m_boneCount; ++i)
			{
				skeleton->m_nodes[i] = m_importAssistor->bones[i];
				skeleton->m_bones[i] = skeleton->m_nodes[i]->m_localTransform;
			}
			for (i = 0; i < skeleton->m_boneCount; ++i)
			{
				skeleton->m_parentIndex[i] = -1;
				for (int j = 0; j < (int)skeleton->m_boneCount; ++j)
				{
					if (skeleton->m_nodes[i]->m_parent == skeleton->m_nodes[j])
					{
						skeleton->m_parentIndex[i] = j;
						break;
					}
				}
			}

			ExtractSkeleton(skeleton, scene);

			m_skeletons.push_back(skeleton);

			ExtractAnimation(scene);
		}

		m_root->UpdateGlobalTransform();

		delete m_importAssistor;
		m_importAssistor = nullptr;
	}

	sdkManager->Destroy();

	//load textures!
	for (auto texture : m_textures)
	{
		m_threads.push_back(new std::thread([](FBXTexture* t)
		{
			t->data = stbi_load(t->path.c_str(), &t->width, &t->height, &t->format, STBI_default);
			//	t->data = SOIL_load_image(t->path.c_str(), &t->width, &t->height, &t->channels, SOIL_LOAD_AUTO);
			if (!t->data)
				printf("Failed to load texture: %s\n", t->path.c_str());
		}, texture.second));
	}

	for (auto t : m_threads)
		t->join();
	m_threads.clear();

	return true;
}

bool FBXFile::LoadAnimationsOnly(const char* filename, UNIT_SCALE scale /* = FBXFile::UNITS_METER */)
{
	if (m_root)
	{
		printf("Scene already loaded!\n");
		return false;
	}

	FbxManager* sdkManager = nullptr;
	FbxScene* scene = nullptr;

	//The first thing to do is to create the FBX SDK manager which is the 
	//object allocator for almost all the classes in the SDK.
	sdkManager = FbxManager::Create();
	if (!sdkManager)
	{
		printf("Unable to create the FBX SDK manager\n");
		return false;
	}

	//create an IOSettings object
	FbxIOSettings * ios = FbxIOSettings::Create(sdkManager, IOSROOT);
	sdkManager->SetIOSettings(ios);

	//Create an importer.
	FbxImporter* importer = FbxImporter::Create(sdkManager, "");

	//Initialize the importer by providing a filename.
	bool importStatus = importer->Initialize(filename, -1, sdkManager->GetIOSettings());

	if (!importStatus)
	{
		printf("Call to FbxImporter::Initialize() failed:\n\t%s\n", importer->GetStatus().GetErrorString());
		importer->Destroy();
		return false;
	}

	//Create the entity that will hold the scene.
	int fileMajor;
	int fileMinor;
	int fileRevision;
	int SDKMajor;
	int SDKMinor;
	int SDKRevision;

	unsigned int i;
	bool status;

	//Get the file version number generate by the FBX SDK.
	FbxManager::GetFileFormatVersion(SDKMajor, SDKMinor, SDKRevision);
	importer->GetFileVersion(fileMajor, fileMinor, fileRevision);

	scene = FbxScene::Create(sdkManager, "root");

	//Import the scene.
	status = importer->Import(scene);
	importer->Destroy();
	if (!status)
	{
		printf("Unable to open FBX file!\n");
		return false;
	}

	float unitScale = 1;

	//convert scale
	if (scene->GetGlobalSettings().GetSystemUnit() != FbxSystemUnit::sPredefinedUnits[scale])
	{
		const FbxSystemUnit::ConversionOptions lConversionOptions = {
			false, //mConvertRrsNodes
			true, //mConvertAllLimits
			true, //mConvertClusters
			true, //mConvertLightIntensity
			true, //mConvertPhotometricLProperties
			true  //mConvertCameraClipPlanes
		};

		unitScale = (float)(scene->GetGlobalSettings().GetSystemUnit().GetScaleFactor() / FbxSystemUnit::sPredefinedUnits[scale].GetScaleFactor());

		//Convert the scene to meters using the defined options.
		FbxSystemUnit::sPredefinedUnits[scale].ConvertScene(scene, lConversionOptions);
	}

	//convert the scene to OpenGL axis (right-handed Y up)
	FbxAxisSystem::OpenGL.ConvertScene(scene);

	FbxNode* node = scene->GetRootNode();

	if (node)
	{
		//store the folder path of the scene
		m_path = filename;
		long lastForward = m_path.find_last_of('/');
		long lastBackward = m_path.find_last_of('\\');
		if (lastForward > lastBackward)
			m_path.resize(lastForward + 1);
		else if (lastBackward)
			m_path.resize(lastBackward + 1);
		else
			m_path = "";

		m_importAssistor = new ImportAssistor();

		m_importAssistor->scene = scene;
		m_importAssistor->evaluator = scene->GetAnimationEvaluator();
		m_importAssistor->importer = importer;
		m_importAssistor->loadTextures = false;
		m_importAssistor->loadAnimations = true;
		m_importAssistor->loadAnimationOnly = true;
		m_importAssistor->unitScale = unitScale;

		m_root = new FBXNode();
		m_root->m_name = "root";
		m_root->m_globalTransform = m_root->m_localTransform = mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

		//grab the ambient light data from the scene
		for (i = 0; i < (unsigned int)node->GetChildCount(); ++i)
			GatherBones((void*)node->GetChild(i));

		//extract children
		for (i = 0; i < (unsigned int)node->GetChildCount(); ++i)
			ExtractObject(m_root, (void*)node->GetChild(i));

		if (m_importAssistor->bones.size() > 0)
		{
			FBXSkeleton* skeleton = new FBXSkeleton();
			skeleton->m_boneCount = (unsigned int)m_importAssistor->bones.size();
			skeleton->m_nodes = new FBXNode *[skeleton->m_boneCount];
			skeleton->m_bones = new mat4[skeleton->m_boneCount];
			skeleton->m_bindPoses = new mat4[skeleton->m_boneCount];
			skeleton->m_parentIndex = new int[skeleton->m_boneCount];

			for (i = 0; i < skeleton->m_boneCount; ++i)
			{
				skeleton->m_nodes[i] = m_importAssistor->bones[i];
				skeleton->m_bones[i] = skeleton->m_nodes[i]->m_localTransform;
			}
			for (i = 0; i < skeleton->m_boneCount; ++i)
			{
				skeleton->m_parentIndex[i] = -1;
				for (int j = 0; j < (int)skeleton->m_boneCount; ++j)
				{
					if (skeleton->m_nodes[i]->m_parent == skeleton->m_nodes[j])
					{
						skeleton->m_parentIndex[i] = j;
						break;
					}
				}
			}

			ExtractSkeleton(skeleton, scene);

			m_skeletons.push_back(skeleton);

			ExtractAnimation(scene);
		}

		m_root->UpdateGlobalTransform();

		delete m_importAssistor;
		m_importAssistor = nullptr;
	}

	sdkManager->Destroy();

	return true;
}

void FBXFile::ExtractObject(FBXNode* parent, void* object)
{
	FbxNode* fbxNode = (FbxNode*)object;

	FBXNode* node = nullptr;

	FbxNodeAttribute::EType attributeType;
	int i;

	bool isBone = false;

	if (fbxNode->GetNodeAttribute())
	{
		attributeType = (fbxNode->GetNodeAttribute()->GetAttributeType());

		switch (attributeType)
		{
		case FbxNodeAttribute::eSkeleton:
			isBone = true;
			break;
		case FbxNodeAttribute::eMesh:
		{
			if (!m_importAssistor->loadAnimationOnly)
			{
				if (fbxNode->GetMaterialCount() > 1)
				{
					node = new FBXNode();
					ExtractMeshes(fbxNode, node);
				}
				else
				{
					node = new FBXMeshNode();
					ExtractMeshes(fbxNode, node);
				}
			}
			break;
		}
		case FbxNodeAttribute::eCamera:
		{
			if (!m_importAssistor->loadAnimationOnly)
			{
				node = new FBXCameraNode();
				ExtractCamera((FBXCameraNode*)node, fbxNode);

				node->m_name = fbxNode->GetName();

				m_cameras[node->m_name] = (FBXCameraNode*)node;
			}
			break;
		}
		case FbxNodeAttribute::eLight:
		{
			if (!m_importAssistor->loadAnimationOnly)
			{
				node = new FBXLightNode();
				ExtractLight((FBXLightNode*)node, fbxNode);

				node->m_name = fbxNode->GetName();

				m_lights[node->m_name] = (FBXLightNode*)node;
			}
			break;
		}
		default:
			break;
		}
	}


	//if null then use it as a plain 3D node
	if (!node)
	{
		node = new FBXNode();
		node->m_name = fbxNode->GetName();
	}

	//add to parent's children and update parent
	parent->m_children.push_back(node);
	node->m_parent = parent;

	//build local transform
	//use anim evaluator as bones store transforms in a different spot
	FbxAMatrix local = m_importAssistor->evaluator->GetNodeLocalTransform(fbxNode);

	node->m_localTransform = mat4(local[0][0], local[0][1], local[0][2], local[0][3], local[1][0], local[1][1], local[1][2], local[1][3], local[2][0], local[2][1], local[2][2], local[2][3], 
										local[3][0], local[3][1], local[3][2], local[3][3]);

	if (m_importAssistor->loadAnimations && isBone)
		m_importAssistor->bones.push_back(node);

	//children
	for (i = 0; i < fbxNode->GetChildCount(); i++)
		ExtractObject(node, (void*)fbxNode->GetChild(i));
}

void FBXFile::ExtractMeshes(void* object, void* aieNode)
{
	FbxNode* fbxNode = (FbxNode*)object;
	FbxMesh* fbxMesh = (FbxMesh*)fbxNode->GetNodeAttribute();

	int i;
	int j;
	int k;
	int l;
	int polygonCount = fbxMesh->GetPolygonCount();
	FbxVector4* controlPoints = fbxMesh->GetControlPoints();
	FbxGeometryElementMaterial* materialElement = fbxMesh->GetElementMaterial(0);

	int materialCount = fbxNode->GetMaterialCount();

	FBXMeshNode** meshes = new FBXMeshNode *[materialCount];
	if (materialCount == 1)
		meshes[0] = (FBXMeshNode*)aieNode;
	else
	{
		for (j = 0; j < materialCount; ++j)
		{
			meshes[j] = new FBXMeshNode();
			meshes[j]->m_vertexAttributes = 0;
		}
	}

	unsigned int vertexIndex[4] = {};
	FBXVertex vertexQuad[4];

	unsigned int* nextIndex = new unsigned int[materialCount];
	for (j = 0; j < materialCount; ++j)
		nextIndex[j] = 0;

	FbxGeometryElementVertexColor* fbxColours = fbxMesh->GetElementVertexColor(0);
	FbxGeometryElementUV* fbxTexCoord0 = fbxMesh->GetElementUV(0);
	FbxGeometryElementUV* fbxTexCoord1 = fbxMesh->GetElementUV(1);
	FbxGeometryElementNormal* fbxNormal = fbxMesh->GetElementNormal(0);

	//gather skinning info
	FbxSkin* fbxSkin = (FbxSkin*)fbxMesh->GetDeformer(0, FbxDeformer::eSkin);
	int skinClusterCount = fbxSkin ? fbxSkin->GetClusterCount() : 0;
	FbxCluster** skinClusters = nullptr;
	int* skinClusterBoneIndices = nullptr;
	if (skinClusterCount > 0)
	{
		skinClusters = new FbxCluster *[skinClusterCount];
		skinClusterBoneIndices = new int[skinClusterCount];

		for (i = 0; i < skinClusterCount; ++i)
		{
			skinClusters[i] = fbxSkin->GetCluster(i);
			if (!skinClusters[i]->GetLink())
				skinClusterBoneIndices[i] = -1;
			else
				skinClusterBoneIndices[i] = m_importAssistor->boneIndexList[skinClusters[i]->GetLink()->GetName()];
		}
	}

	//needed for accessing certain vertex properties
	int vertexId = 0;

	//process each polygon (tris and quads only)
	for (i = 0; i < polygonCount; ++i)
	{
		int polygonSize = fbxMesh->GetPolygonSize(i);

		int material = materialElement->GetIndexArray().GetAt(i);

		for (j = 0; j < polygonSize && j < 4; ++j)
		{
			FBXVertex vertex;

			int controlPointIndex = fbxMesh->GetPolygonVertex(i, j);

			FbxVector4 pos = controlPoints[controlPointIndex];
			vertex.m_position.x = (float)pos[0];
			vertex.m_position.y = (float)pos[1];
			vertex.m_position.z = (float)pos[2];
			vertex.m_position.w = 1;

			meshes[material]->m_vertexAttributes |= FBXVertex::ePOSITION;

			//extract colour data
			if (fbxColours)
			{
				switch (fbxColours->GetMappingMode())
				{
				case FbxGeometryElement::eByControlPoint:
					switch (fbxColours->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
					{
						FbxColor colour = fbxColours->GetDirectArray().GetAt(controlPointIndex);
						meshes[material]->m_vertexAttributes |= FBXVertex::eCOLOUR;

						vertex.m_colour.x = (float)colour.mRed;
						vertex.m_colour.y = (float)colour.mGreen;
						vertex.m_colour.z = (float)colour.mBlue;
						vertex.m_colour.w = (float)colour.mAlpha;
					}
					break;
					case FbxGeometryElement::eIndexToDirect:
					{
						int id = fbxColours->GetIndexArray().GetAt(controlPointIndex);
						FbxColor colour = fbxColours->GetDirectArray().GetAt(id);
						meshes[material]->m_vertexAttributes |= FBXVertex::eCOLOUR;

						vertex.m_colour.x = (float)colour.mRed;
						vertex.m_colour.y = (float)colour.mGreen;
						vertex.m_colour.z = (float)colour.mBlue;
						vertex.m_colour.w = (float)colour.mAlpha;
					}
					break;
					default:
						break; //other reference modes not shown here!
					}
					break;

				case FbxGeometryElement::eByPolygonVertex:
				{
					switch (fbxColours->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
					{
						FbxColor colour = fbxColours->GetDirectArray().GetAt(vertexId);
						meshes[material]->m_vertexAttributes |= FBXVertex::eCOLOUR;

						vertex.m_colour.x = (float)colour.mRed;
						vertex.m_colour.y = (float)colour.mGreen;
						vertex.m_colour.z = (float)colour.mBlue;
						vertex.m_colour.w = (float)colour.mAlpha;
					}
					break;
					case FbxGeometryElement::eIndexToDirect:
					{
						int id = fbxColours->GetIndexArray().GetAt(vertexId);
						FbxColor colour = fbxColours->GetDirectArray().GetAt(id);
						meshes[material]->m_vertexAttributes |= FBXVertex::eCOLOUR;

						vertex.m_colour.x = (float)colour.mRed;
						vertex.m_colour.y = (float)colour.mGreen;
						vertex.m_colour.z = (float)colour.mBlue;
						vertex.m_colour.w = (float)colour.mAlpha;
					}
					break;
					default:
						break;	//other reference modes not shown here!
					}
				}
				break;

				case FbxGeometryElement::eByPolygon:	//doesn't make much sense for UVs
				case FbxGeometryElement::eAllSame:	//doesn't make much sense for UVs
				case FbxGeometryElement::eNone:	//doesn't make much sense for UVs
					break;
				default:
					break;
				}
			}
			//extract first texture coordinate set
			if (fbxTexCoord0)
			{
				switch (fbxTexCoord0->GetMappingMode())
				{
				case FbxGeometryElement::eByControlPoint:
					switch (fbxTexCoord0->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
					{
						FbxVector2 uv = fbxTexCoord0->GetDirectArray().GetAt(controlPointIndex);
						meshes[material]->m_vertexAttributes |= FBXVertex::eTEXCOORD1;

						vertex.m_texCoord1.x = (float)uv[0];
						if (m_importAssistor->flipTextureY)
							vertex.m_texCoord1.y = 1.f - (float)uv[1];
						else
							vertex.m_texCoord1.y = (float)uv[1];
					}
					break;
					case FbxGeometryElement::eIndexToDirect:
					{
						int id = fbxTexCoord0->GetIndexArray().GetAt(controlPointIndex);
						FbxVector2 uv = fbxTexCoord0->GetDirectArray().GetAt(id);
						meshes[material]->m_vertexAttributes |= FBXVertex::eTEXCOORD1;

						vertex.m_texCoord1.x = (float)uv[0];
						if (m_importAssistor->flipTextureY)
							vertex.m_texCoord1.y = 1.f - (float)uv[1];
						else
							vertex.m_texCoord1.y = (float)uv[1];
					}
					break;
					default:
						break; //other reference modes not shown here!
					}
					break;

				case FbxGeometryElement::eByPolygonVertex:
				{
					int textureUVIndex = fbxMesh->GetTextureUVIndex(i, j);
					switch (fbxTexCoord0->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
					case FbxGeometryElement::eIndexToDirect:
					{
						FbxVector2 uv = fbxTexCoord0->GetDirectArray().GetAt(textureUVIndex);
						meshes[material]->m_vertexAttributes |= FBXVertex::eTEXCOORD1;

						vertex.m_texCoord1.x = (float)uv[0];
						if (m_importAssistor->flipTextureY)
							vertex.m_texCoord1.y = 1.f - (float)uv[1];
						else
							vertex.m_texCoord1.y = (float)uv[1];
					}
					break;
					default:
						break; //other reference modes not shown here!
					}
				}
				break;

				case FbxGeometryElement::eByPolygon: //doesn't make much sense for UVs
				case FbxGeometryElement::eAllSame:   //doesn't make much sense for UVs
				case FbxGeometryElement::eNone:       //doesn't make much sense for UVs
					break;
				default:
					break;
				}
			}

			//extract second coordinate set
			if (fbxTexCoord1)
			{
				switch (fbxTexCoord1->GetMappingMode())
				{
				case FbxGeometryElement::eByControlPoint:
					switch (fbxTexCoord1->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
					{
						FbxVector2 uv = fbxTexCoord1->GetDirectArray().GetAt(controlPointIndex);
						meshes[material]->m_vertexAttributes |= FBXVertex::eTEXCOORD2;

						vertex.m_texCoord2.x = (float)uv[0];
						if (m_importAssistor->flipTextureY)
							vertex.m_texCoord2.y = 1.f - (float)uv[1];
						else
							vertex.m_texCoord2.y = (float)uv[1];
					}
					break;
					case FbxGeometryElement::eIndexToDirect:
					{
						int id = fbxTexCoord1->GetIndexArray().GetAt(controlPointIndex);
						FbxVector2 uv = fbxTexCoord1->GetDirectArray().GetAt(id);
						meshes[material]->m_vertexAttributes |= FBXVertex::eTEXCOORD2;

						vertex.m_texCoord2.x = (float)uv[0];
						if (m_importAssistor->flipTextureY)
							vertex.m_texCoord2.y = 1.f - (float)uv[1];
						else
							vertex.m_texCoord2.y = (float)uv[1];
					}
					break;
					default:
						break; //other reference modes not shown here!
					}
					break;

				case FbxGeometryElement::eByPolygonVertex:
				{
					int textureUVIndex = fbxMesh->GetTextureUVIndex(i, j);
					switch (fbxTexCoord1->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
					case FbxGeometryElement::eIndexToDirect:
					{
						FbxVector2 uv = fbxTexCoord1->GetDirectArray().GetAt(textureUVIndex);
						meshes[material]->m_vertexAttributes |= FBXVertex::eTEXCOORD2;

						vertex.m_texCoord2.x = (float)uv[0];
						if (m_importAssistor->flipTextureY)
							vertex.m_texCoord2.y = 1.f - (float)uv[1];
						else
							vertex.m_texCoord2.y = (float)uv[1];
					}
					break;
					default:
						break; //other reference modes not shown here!
					}
				}
				break;

				case FbxGeometryElement::eByPolygon: //doesn't make much sense for UVs
				case FbxGeometryElement::eAllSame:   //doesn't make much sense for UVs
				case FbxGeometryElement::eNone:       //doesn't make much sense for UVs
					break;
				default:
					break;
				}
			}

			//extract normal data
			if (fbxNormal)
			{
				if (fbxNormal->GetMappingMode() == FbxGeometryElement::eByControlPoint)
				{
					switch (fbxNormal->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
					{
						FbxVector4 normal = fbxNormal->GetDirectArray().GetAt(controlPointIndex);
						meshes[material]->m_vertexAttributes |= FBXVertex::eNORMAL;

						vertex.m_normal.x = (float)normal[0];
						vertex.m_normal.y = (float)normal[1];
						vertex.m_normal.z = (float)normal[2];
						vertex.m_normal.w = 0;
					}
					break;
					case FbxGeometryElement::eIndexToDirect:
					{
						int id = fbxNormal->GetIndexArray().GetAt(controlPointIndex);
						FbxVector4 normal = fbxNormal->GetDirectArray().GetAt(id);
						meshes[material]->m_vertexAttributes |= FBXVertex::eNORMAL;

						vertex.m_normal.x = (float)normal[0];
						vertex.m_normal.y = (float)normal[1];
						vertex.m_normal.z = (float)normal[2];
						vertex.m_normal.w = 0;
					}
					break;
					default:
						break; //other reference modes not shown here!
					}
				}
				else if (fbxNormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
				{
					switch (fbxNormal->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
					{
						FbxVector4 normal = fbxNormal->GetDirectArray().GetAt(vertexId);
						meshes[material]->m_vertexAttributes |= FBXVertex::eNORMAL;

						vertex.m_normal.x = (float)normal[0];
						vertex.m_normal.y = (float)normal[1];
						vertex.m_normal.z = (float)normal[2];
						vertex.m_normal.w = 0;
					}
					break;
					case FbxGeometryElement::eIndexToDirect:
					{
						int id = fbxNormal->GetIndexArray().GetAt(vertexId);
						FbxVector4 normal = fbxNormal->GetDirectArray().GetAt(id);
						meshes[material]->m_vertexAttributes |= FBXVertex::eNORMAL;

						vertex.m_normal.x = (float)normal[0];
						vertex.m_normal.y = (float)normal[1];
						vertex.m_normal.z = (float)normal[2];
						vertex.m_normal.w = 0;
					}
					break;
					default:
						break; //other reference modes not shown here!
					}
				}
			}

			//gather skinning data (slow but can't find any other way, yet!)
			if (fbxSkin)
			{
				for (k = 0; k != skinClusterCount; ++k)
				{
					if (skinClusterBoneIndices[k] == -1)
						continue;

					int indexCount = skinClusters[k]->GetControlPointIndicesCount();
					int* indices = skinClusters[k]->GetControlPointIndices();
					double* weights = skinClusters[k]->GetControlPointWeights();

					for (l = 0; l < indexCount; l++)
					{
						if (controlPointIndex == indices[l])
						{
							//add weight and index
							if (!vertex.m_weights.x)
							{
								vertex.m_weights.x = (float)weights[l];
								vertex.m_indices.x = (float)skinClusterBoneIndices[k];
							}
							else if (!vertex.m_weights.y)
							{
								vertex.m_weights.y = (float)weights[l];
								vertex.m_indices.y = (float)skinClusterBoneIndices[k];
							}
							else if (!vertex.m_weights.z)
							{
								vertex.m_weights.z = (float)weights[l];
								vertex.m_indices.z = (float)skinClusterBoneIndices[k];
							}
							else
							{
								vertex.m_weights.w = (float)weights[l];
								vertex.m_indices.w = (float)skinClusterBoneIndices[k];
							}
						}
					}
				}
			}

			vertex.m_index[0] = nextIndex[material]++;
			vertexQuad[j] = vertex;
			meshes[material]->m_meshVertices.push_back(vertex);
			vertexId++;
		}

		//add triangle indices
		meshes[material]->m_meshIndices.push_back(vertexQuad[0].m_index[0]);
		meshes[material]->m_meshIndices.push_back(vertexQuad[1].m_index[0]);
		meshes[material]->m_meshIndices.push_back(vertexQuad[2].m_index[0]);

		//handle quads
		if (polygonSize == 4)
		{
			meshes[material]->m_meshIndices.push_back(vertexQuad[3].m_index[0]);

			vertexQuad[0].m_index[0] = nextIndex[material]++;
			meshes[material]->m_meshVertices.push_back(vertexQuad[0]);
			meshes[material]->m_meshIndices.push_back(vertexQuad[0].m_index[0]);

			vertexQuad[2].m_index[0] = nextIndex[material]++;
			meshes[material]->m_meshVertices.push_back(vertexQuad[2]);
			meshes[material]->m_meshIndices.push_back(vertexQuad[2].m_index[0]);
		}
	}

	for (int i = 0; i < materialCount; ++i)
		m_threads.push_back(new std::thread(OptimiseMesh, meshes[i]));

	//set mesh names, vertex attributes, extract material and add to mesh map
	for (j = 0; j < materialCount; ++j)
	{
		meshes[j]->m_name = fbxNode->GetName();

		//append material name to mesh name
		if (materialCount > 1)
			meshes[j]->m_name += fbxNode->GetMaterial(j)->GetName();

		meshes[j]->m_material = ExtractMaterial(fbxMesh, j);
		m_meshes.push_back(meshes[j]);
	}

	//if there is a single mesh return it, else make a new parent node and return that
	if (materialCount > 1)
	{
		FBXNode* node = (FBXNode*)aieNode;
		node->m_name = fbxNode->GetName();
		for (j = 0; j < materialCount; ++j)
		{
			node->m_children.push_back(meshes[j]);
			meshes[j]->m_parent = node;
		}
	}

	delete[] skinClusters;
	delete[] skinClusterBoneIndices;

	delete[] meshes;
	delete[] nextIndex;
}

void FBXFile::OptimiseMesh(FBXMeshNode* mesh)
{
	//sort the vertex array so all common verts are adjacent in the array
	std::sort(mesh->m_meshVertices.begin(), mesh->m_meshVertices.end());

	unsigned int forwardIter = 1;
	int j = 0;

	while (forwardIter < mesh->m_meshVertices.size())
	{
		if (mesh->m_meshVertices[j] == mesh->m_meshVertices[forwardIter])
		{
			//if the adjacent verts are equal make all the duplicate vert's indicies point at the first one in the vector
			mesh->m_meshIndices[mesh->m_meshVertices[forwardIter].m_index[0]] = j;
			++forwardIter;
		}
		else
		{
			//if they aren't duplicates, update the index to point at the vert's post sort position in the vector
			mesh->m_meshIndices[mesh->m_meshVertices[j].m_index[0]] = j;
			++j;
			//then push the current forward iterator back
			//not sure if checking if j != forward pointer would be faster here.
			mesh->m_meshVertices[j] = mesh->m_meshVertices[forwardIter];
			mesh->m_meshIndices[mesh->m_meshVertices[forwardIter].m_index[0]] = j;
			++forwardIter;
		}
	}
	mesh->m_meshVertices.resize(j + 1);

	if ((mesh->m_vertexAttributes & FBXVertex::eTEXCOORD1))
	{
		mesh->m_vertexAttributes |= FBXVertex::eTANGENT | FBXVertex::eBINORMAL;
		CalculateTangentsBinormals(mesh->m_meshVertices, mesh->m_meshIndices);
	}
}

void FBXFile::ExtractLight(FBXLightNode* light, void* object)
{
	FbxNode* fbxNode = (FbxNode*)object;
	FbxLight* fbxLight = (FbxLight*)fbxNode->GetNodeAttribute();

	//get type, if on, and colour
	light->m_type = (FBXLightNode::LightType)fbxLight->LightType.Get();
	light->m_on = fbxLight->CastLight.Get();
	light->m_lightColour.x = (float)fbxLight->Color.Get()[0];
	light->m_lightColour.y = (float)fbxLight->Color.Get()[1];
	light->m_lightColour.z = (float)fbxLight->Color.Get()[2];
	light->m_lightColour.w = (float)fbxLight->Intensity.Get();

	//get spot light angles (will return data even for non-spotlights)
	light->m_innerAngle = (float)fbxLight->InnerAngle.Get() * (pi<float>() / 180);
	light->m_outerAngle = (float)fbxLight->OuterAngle.Get() * (pi<float>() / 180);

	//get falloff data (none,linear, quadratic), cubic is ignored
	switch (fbxLight->DecayType.Get())
	{
	case 0:
		light->m_attenuation = vec4(1, 0, 0, 0);
		break;
	case 1:
		break;
		light->m_attenuation = vec4(0, 1, 0, 0);
	case 2:
		break;
		light->m_attenuation = vec4(0, 0, 1, 0);
	default:
		break;
	};
}

void FBXFile::ExtractCamera(FBXCameraNode* camera, void* object)
{
	FbxNode* fbxNode = (FbxNode*)object;
	FbxCamera* fbxCamera = (FbxCamera*)fbxNode->GetNodeAttribute();

	//get field of view
	if (fbxCamera->ProjectionType.Get() != FbxCamera::eOrthogonal)
		camera->m_fieldOfView = (float)fbxCamera->FieldOfView.Get() * (pi<float>() / 180);
	else
		camera->m_fieldOfView = 0;

	//get aspect ratio if one was defined
	if (fbxCamera->GetAspectRatioMode() != FbxCamera::eWindowSize)
		camera->m_aspectRatio = (float)fbxCamera->AspectWidth.Get() / (float)fbxCamera->AspectHeight.Get();
	else
		camera->m_aspectRatio = 0;

	//get near/far
	camera->m_near = (float)fbxCamera->NearPlane.Get();
	camera->m_far = (float)fbxCamera->FarPlane.Get();

	//build view matrix
	vec3 eye;
	vec3 to;
	vec3 up;

	eye.x = (float)fbxCamera->Position.Get()[0];
	eye.y = (float)fbxCamera->Position.Get()[1];
	eye.z = (float)fbxCamera->Position.Get()[2];

	if (fbxNode->GetTarget())
	{
		to.x = (float)fbxNode->GetTarget()->LclTranslation.Get()[0];
		to.y = (float)fbxNode->GetTarget()->LclTranslation.Get()[1];
		to.z = (float)fbxNode->GetTarget()->LclTranslation.Get()[2];
	}
	else
	{
		to.x = (float)fbxCamera->InterestPosition.Get()[0];
		to.y = (float)fbxCamera->InterestPosition.Get()[1];
		to.z = (float)fbxCamera->InterestPosition.Get()[2];
	}

	if (fbxNode->GetTargetUp())
	{
		up.x = (float)fbxNode->GetTargetUp()->LclTranslation.Get()[0];
		up.y = (float)fbxNode->GetTargetUp()->LclTranslation.Get()[1];
		up.z = (float)fbxNode->GetTargetUp()->LclTranslation.Get()[2];

	}
	else
	{
		up.x = (float)fbxCamera->UpVector.Get()[0];
		up.y = (float)fbxCamera->UpVector.Get()[1];
		up.z = (float)fbxCamera->UpVector.Get()[2];
	}

	camera->m_viewMatrix = glm::lookAt(eye, to, up);
}

FBXMaterial* FBXFile::ExtractMaterial(void* mesh, int materialIndex)
{
	FbxGeometry* geometry = (FbxGeometry*)mesh;
	FbxNode* node = geometry->GetNode();
	FbxSurfaceMaterial* surfaceMaterial = node->GetMaterial(materialIndex);

	//check if material already loaded, else create new material
	m_materialMutex.lock();
	auto iter = m_materials.find(surfaceMaterial->GetName());
	if (iter != m_materials.end())
	{
		m_materialMutex.unlock();
		return iter->second;
	}
	else
	{
		FBXMaterial* material = new FBXMaterial;
		material->name = surfaceMaterial->GetName();

		//get the implementation to see if it's a hardware shader.
		const FbxImplementation* implementation = GetImplementation(surfaceMaterial, FBXSDK_IMPLEMENTATION_HLSL);
		if (!implementation)
			implementation = GetImplementation(surfaceMaterial, FBXSDK_IMPLEMENTATION_CGFX);
		if (implementation)
		{
			FbxBindingTable const* rootTable = implementation->GetRootTable();
			FbxString fileName = rootTable->DescAbsoluteURL.Get();
			FbxString techniqueName = rootTable->DescTAG.Get();

			printf("Unsupported hardware shader material!\n");
			printf("\tFile: %s\n", fileName.Buffer());
			printf("\tTechnique: %s\n\n", techniqueName.Buffer());
		}
		else if (surfaceMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
		{
			//We found a Phong material
			FbxSurfacePhong* phong = (FbxSurfacePhong*)surfaceMaterial;

			material->ambient.x = (float)phong->Ambient.Get()[0];
			material->ambient.y = (float)phong->Ambient.Get()[1];
			material->ambient.z = (float)phong->Ambient.Get()[2];
			material->ambient.w = (float)phong->AmbientFactor.Get();

			material->diffuse.x = (float)phong->Diffuse.Get()[0];
			material->diffuse.y = (float)phong->Diffuse.Get()[1];
			material->diffuse.z = (float)phong->Diffuse.Get()[2];
			material->diffuse.w = (float)phong->TransparencyFactor.Get();

			material->specular.x = (float)phong->Specular.Get()[0];
			material->specular.y = (float)phong->Specular.Get()[1];
			material->specular.z = (float)phong->Specular.Get()[2];
			material->specular.w = (float)phong->Shininess.Get();

			material->emissive.x = (float)phong->Emissive.Get()[0];
			material->emissive.y = (float)phong->Emissive.Get()[1];
			material->emissive.z = (float)phong->Emissive.Get()[2];
			material->emissive.w = (float)phong->EmissiveFactor.Get();
		}
		else if (surfaceMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
		{
			FbxSurfaceLambert* lambert = (FbxSurfaceLambert*)surfaceMaterial;

			material->ambient.x = (float)lambert->Ambient.Get()[0];
			material->ambient.y = (float)lambert->Ambient.Get()[1];
			material->ambient.z = (float)lambert->Ambient.Get()[2];
			material->ambient.w = (float)lambert->AmbientFactor.Get();

			material->diffuse.x = (float)lambert->Diffuse.Get()[0];
			material->diffuse.y = (float)lambert->Diffuse.Get()[1];
			material->diffuse.z = (float)lambert->Diffuse.Get()[2];
			material->diffuse.w = (float)lambert->TransparencyFactor.Get();

			//No specular in lambert materials
			material->specular.x = 0;
			material->specular.y = 0;
			material->specular.z = 0;
			material->specular.w = 0;

			material->emissive.x = (float)lambert->Emissive.Get()[0];
			material->emissive.y = (float)lambert->Emissive.Get()[1];
			material->emissive.z = (float)lambert->Emissive.Get()[2];
			material->emissive.w = (float)lambert->EmissiveFactor.Get();
		}
		else
			printf("Unknown type of Material: %s\n", surfaceMaterial->GetClassId().GetName());

		unsigned int textureLookup[] =
		{
			FbxLayerElement::eTextureDiffuse - FbxLayerElement::sTypeTextureStartIndex,
			FbxLayerElement::eTextureAmbient - FbxLayerElement::sTypeTextureStartIndex,
			FbxLayerElement::eTextureEmissive - FbxLayerElement::sTypeTextureStartIndex,
			FbxLayerElement::eTextureSpecular - FbxLayerElement::sTypeTextureStartIndex,
			FbxLayerElement::eTextureShininess - FbxLayerElement::sTypeTextureStartIndex,
			FbxLayerElement::eTextureNormalMap - FbxLayerElement::sTypeTextureStartIndex,
			FbxLayerElement::eTextureTransparency - FbxLayerElement::sTypeTextureStartIndex,
			FbxLayerElement::eTextureDisplacement - FbxLayerElement::sTypeTextureStartIndex,
		};

		if (m_importAssistor->loadTextures)
		{
			for (unsigned int i = 0; i < FBXMaterial::TextureTypes_Count; ++i)
			{
				FbxProperty property = surfaceMaterial->FindProperty(FbxLayerElement::sTextureChannelNames[textureLookup[i]]);
				if (property.IsValid() && property.GetSrcObjectCount<FbxTexture>() > 0)
				{
					FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(property.GetSrcObject<FbxTexture>(0));
					if (fileTexture)
					{
						const char* lastForward = strrchr(fileTexture->GetFileName(), '/');
						const char* lastBackward = strrchr(fileTexture->GetFileName(), '\\');
						const char* filename = fileTexture->GetFileName();

						material->textureRotation[i] = (float)fileTexture->GetRotationW();
						material->textureTiling[i].x = (float)fileTexture->GetScaleU();
						material->textureTiling[i].y = (float)fileTexture->GetScaleV();
						material->textureOffsets[i].x = (float)fileTexture->GetTranslationU();
						material->textureOffsets[i].y = (float)fileTexture->GetTranslationV();

						if (lastForward && lastForward > lastBackward)
							filename = lastForward + 1;
						else if (lastBackward)
							filename = lastBackward + 1;

						std::string fullPath = m_path + filename;

						auto iter = m_textures.find(fullPath);
						if (iter != m_textures.end())
							material->textures[i] = iter->second;
						else
						{
							FBXTexture* texture = new FBXTexture();
							texture->name = filename;
							texture->path = fullPath;
							material->textures[i] = texture;
							m_textures[fullPath] = texture;
						}
					}
				}
			}
		}

		m_materials[material->name] = material;
		m_materialMutex.unlock();
		return material;
	}

	m_materialMutex.unlock();
	return nullptr;
}

void FBXFile::InitialiseOpenGLTextures()
{
	for (auto texture : m_textures)
	{
		//	texture.second->handle = SOIL_create_OGL_texture(texture.second->data, texture.second->width, texture.second->height, texture.second->channels, 
		//		SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_TEXTURE_REPEATS);
		switch (texture.second->format)
		{
		case STBI_grey:
			texture.second->format = GL_LUMINANCE;
			break;
		case STBI_grey_alpha:
			texture.second->format = GL_LUMINANCE_ALPHA;
			break;
		case STBI_rgb:
			texture.second->format = GL_RGB;
			break;
		case STBI_rgb_alpha:
			texture.second->format = GL_RGBA;
			break;
		};

		glGenTextures(1, &texture.second->handle);
		glBindTexture(GL_TEXTURE_2D, texture.second->handle);
		glTexImage2D(GL_TEXTURE_2D, 0, texture.second->format, texture.second->width, texture.second->height, 0, texture.second->format, GL_UNSIGNED_BYTE, texture.second->data);
		//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		//glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void FBXFile::ExtractAnimation(void* scene)
{
	FbxScene* fbxScene = (FbxScene*)scene;

	int animStackCount = fbxScene->GetSrcObjectCount<FbxAnimStack>();

	vector<int> tracks;
	vector<void*> nodes;
	tracks.reserve(128);
	nodes.reserve(128);

	FbxTime frameTime;

	for (int i = 0; i < animStackCount; i++)
	{
		tracks.clear();
		nodes.clear();

		FbxAnimStack* animStack = fbxScene->GetSrcObject<FbxAnimStack>(i);

		FBXAnimation* anim = new FBXAnimation();
		anim->m_name = animStack->GetName();

		//get animated track bone indices and nodes, and calculate start/end frame
		int animLayers = animStack->GetMemberCount(FbxCriteria::ObjectType(FbxAnimLayer::ClassId));
		for (int j = 0; j < animLayers; ++j)
		{
			FbxAnimLayer* animLayer = animStack->GetMember<FbxAnimLayer>(j);
			ExtractAnimationTrack(tracks, animLayer, fbxScene->GetRootNode(), nodes, anim->m_startFrame, anim->m_endFrame);
		}

		//frame count (includes start/end frames)
		unsigned int frameCount = anim->m_endFrame - anim->m_startFrame + 1;

		//allocate tracks and keyframes
		anim->m_trackCount = (unsigned int)tracks.size();
		anim->m_tracks = new FBXTrack[anim->m_trackCount];
		for (unsigned int track = 0; track < anim->m_trackCount; ++track)
		{
			anim->m_tracks[track].m_boneIndex = tracks[track];
			anim->m_tracks[track].m_keyframeCount = frameCount;
			anim->m_tracks[track].m_keyframes = new FBXKeyFrame[frameCount];
		}

		//evaluate all of the animated track keyframes
		//loop by frame first to limit FBX time changes (dreadfully slow!)
		for (unsigned int frame = 0; frame < frameCount; ++frame)
		{
			frameTime.SetFrame(frame + anim->m_startFrame);

			for (unsigned int track = 0; track < anim->m_trackCount; ++track)
			{
				FbxAMatrix localMatrix = m_importAssistor->evaluator->GetNodeLocalTransform((FbxNode*)nodes[track], frameTime);

				FbxQuaternion r = localMatrix.GetQ();
				FbxVector4 t = localMatrix.GetT();
				FbxVector4 s = localMatrix.GetS();

				anim->m_tracks[track].m_keyframes[frame].m_key = frame + anim->m_startFrame;
				anim->m_tracks[track].m_keyframes[frame].m_rotation = glm::quat((float)r[3], (float)r[0], (float)r[1], (float)r[2]);
				anim->m_tracks[track].m_keyframes[frame].m_translation = vec3((float)t[0], (float)t[1], (float)t[2]);
				anim->m_tracks[track].m_keyframes[frame].m_scale = vec3((float)s[0], (float)s[1], (float)s[2]);
			}
		}

		m_animations[anim->m_name] = anim;
	}
}

void FBXFile::ExtractAnimationTrack(vector<int>& tracks, void* layer, void* node, vector<void*>& nodes, unsigned int& startFrame, unsigned int& endFrame)
{
	FbxAnimLayer* fbxAnimLayer = (FbxAnimLayer*)layer;
	FbxNode* fbxNode = (FbxNode*)node;

	//find node index in skeleton
	int boneIndex = -1;
	FBXSkeleton* skeleton = m_skeletons[0];
	for (unsigned int i = 0; i < skeleton->m_boneCount; ++i)
	{
		if (skeleton->m_nodes[i]->m_name == fbxNode->GetName())
		{
			boneIndex = i;
			break;
		}
	}

	//extract bones that have animated properties on them only
	bool hasKeys = false;
	FbxAnimCurve* animCurve = nullptr;
	if (boneIndex >= 0)
	{
		//translation
		animCurve = fbxNode->LclTranslation.GetCurve(fbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
		if (animCurve)
		{
			hasKeys = true;

			unsigned int frame = (unsigned int)animCurve->KeyGetTime(0).GetFrameCount();
			if (frame < startFrame)
				startFrame = frame;

			frame = (unsigned int)animCurve->KeyGetTime(animCurve->KeyGetCount() - 1).GetFrameCount();
			if (frame > endFrame)
				endFrame = frame;
		}
		animCurve = fbxNode->LclTranslation.GetCurve(fbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
		if (animCurve)
		{
			hasKeys = true;

			unsigned int frame = (unsigned int)animCurve->KeyGetTime(0).GetFrameCount();
			if (frame < startFrame)
				startFrame = frame;

			frame = (unsigned int)animCurve->KeyGetTime(animCurve->KeyGetCount() - 1).GetFrameCount();
			if (frame > endFrame)
				endFrame = frame;

		}
		animCurve = fbxNode->LclTranslation.GetCurve(fbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
		if (animCurve)
		{
			hasKeys = true;

			unsigned int frame = (unsigned int)animCurve->KeyGetTime(0).GetFrameCount();
			if (frame < startFrame)
				startFrame = frame;

			frame = (unsigned int)animCurve->KeyGetTime(animCurve->KeyGetCount() - 1).GetFrameCount();
			if (frame > endFrame)
				endFrame = frame;

		}

		//rotation
		animCurve = fbxNode->LclRotation.GetCurve(fbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
		if (animCurve)
		{
			hasKeys = true;

			unsigned int frame = (unsigned int)animCurve->KeyGetTime(0).GetFrameCount();
			if (frame < startFrame)
				startFrame = frame;

			frame = (unsigned int)animCurve->KeyGetTime(animCurve->KeyGetCount() - 1).GetFrameCount();
			if (frame > endFrame)
				endFrame = frame;

		}
		animCurve = fbxNode->LclRotation.GetCurve(fbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
		if (animCurve)
		{
			hasKeys = true;

			unsigned int frame = (unsigned int)animCurve->KeyGetTime(0).GetFrameCount();
			if (frame < startFrame)
				startFrame = frame;

			frame = (unsigned int)animCurve->KeyGetTime(animCurve->KeyGetCount() - 1).GetFrameCount();
			if (frame > endFrame)
				endFrame = frame;

		}
		animCurve = fbxNode->LclRotation.GetCurve(fbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
		if (animCurve)
		{
			hasKeys = true;

			unsigned int frame = (unsigned int)animCurve->KeyGetTime(0).GetFrameCount();
			if (frame < startFrame)
				startFrame = frame;

			frame = (unsigned int)animCurve->KeyGetTime(animCurve->KeyGetCount() - 1).GetFrameCount();
			if (frame > endFrame)
				endFrame = frame;

		}

		//scale
		animCurve = fbxNode->LclScaling.GetCurve(fbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
		if (animCurve)
		{
			hasKeys = true;

			unsigned int frame = (unsigned int)animCurve->KeyGetTime(0).GetFrameCount();
			if (frame < startFrame)
				startFrame = frame;

			frame = (unsigned int)animCurve->KeyGetTime(animCurve->KeyGetCount() - 1).GetFrameCount();
			if (frame > endFrame)
				endFrame = frame;

		}
		animCurve = fbxNode->LclScaling.GetCurve(fbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
		if (animCurve)
		{
			hasKeys = true;

			unsigned int frame = (unsigned int)animCurve->KeyGetTime(0).GetFrameCount();
			if (frame < startFrame)
				startFrame = frame;

			frame = (unsigned int)animCurve->KeyGetTime(animCurve->KeyGetCount() - 1).GetFrameCount();
			if (frame > endFrame)
				endFrame = frame;

		}
		animCurve = fbxNode->LclScaling.GetCurve(fbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
		if (animCurve)
		{
			hasKeys = true;

			unsigned int frame = (unsigned int)animCurve->KeyGetTime(0).GetFrameCount();
			if (frame < startFrame)
				startFrame = frame;

			frame = (unsigned int)animCurve->KeyGetTime(animCurve->KeyGetCount() - 1).GetFrameCount();
			if (frame > endFrame)
				endFrame = frame;
		}

		if (hasKeys)
		{
			nodes.push_back(fbxNode);
			tracks.push_back(boneIndex);
		}
	}

	for (int i = 0; i < fbxNode->GetChildCount(); ++i)
		ExtractAnimationTrack(tracks, fbxAnimLayer, fbxNode->GetChild(i), nodes, startFrame, endFrame);
}

void FBXFile::ExtractSkeleton(FBXSkeleton* skeleton, void* scene)
{
	FbxScene* fbxScene = (FbxScene*)scene;

	int poseCount = fbxScene->GetPoseCount();

	for (int i = 0; i < poseCount; ++i)
	{
		FbxPose* pose = fbxScene->GetPose(i);

		for (int j = 0; j < pose->GetCount(); ++j)
		{
			float scaleFactor = m_importAssistor->unitScale;

			FbxMatrix scale(scaleFactor, 0, 0, 0, 0, scaleFactor, 0, 0, 0, 0, scaleFactor, 0, 0, 0, 0, 1);

			FbxMatrix matrix = pose->GetMatrix(j);
			FbxMatrix bindMatrix = matrix.Inverse() * scale;

			for (unsigned int k = 0; k < skeleton->m_boneCount; ++k)
			{
				if (skeleton->m_nodes[k]->m_name == pose->GetNodeName(j).GetCurrentName())
				{
					FbxVector4 row0 = bindMatrix.GetRow(0);
					FbxVector4 row1 = bindMatrix.GetRow(1);
					FbxVector4 row2 = bindMatrix.GetRow(2);
					FbxVector4 row3 = bindMatrix.GetRow(3);

					skeleton->m_bindPoses[k][0] = vec4((float)row0[0], (float)row0[1], (float)row0[2], (float)row0[3]);
					skeleton->m_bindPoses[k][1] = vec4((float)row1[0], (float)row1[1], (float)row1[2], (float)row1[3]);
					skeleton->m_bindPoses[k][2] = vec4((float)row2[0], (float)row2[1], (float)row2[2], (float)row2[3]);
					skeleton->m_bindPoses[k][3] = vec4((float)row3[0], (float)row3[1], (float)row3[2], (float)row3[3]);
				}
			}
		}
	}
}

void FBXSkeleton::Evaluate(const FBXAnimation* animation, float time, bool loop, float FPS)
{
	if (animation)
	{
		//determine frame we're on
		int totalFrames = animation->m_endFrame - animation->m_startFrame;
		float animDuration = totalFrames / FPS;

		//get time through frame
		float frameTime = 0;
		if (loop)
			frameTime = max(glm::mod(time, animDuration), 0.f);
		else
			frameTime = glm::min(max(time, 0.f), animDuration);

		const FBXTrack* track = nullptr;
		const FBXKeyFrame* start = nullptr;
		const FBXKeyFrame* end = nullptr;

		for (unsigned int i = 0; i < animation->m_trackCount; ++i)
		{
			track = &(animation->m_tracks[i]);

			start = nullptr;
			end = nullptr;
			float startTime;
			float endTime;

			//determine the two keyframes we're between
			for (unsigned int j = 0; j < track->m_keyframeCount - 1; ++j)
			{
				startTime = (track->m_keyframes[j].m_key - animation->m_startFrame) / FPS;
				endTime = (track->m_keyframes[j + 1].m_key - animation->m_startFrame) / FPS;

				if (startTime <= frameTime && endTime >= frameTime)
				{
					//found?
					start = &(track->m_keyframes[j]);
					end = &(track->m_keyframes[j + 1]);
					break;
				}
			}

			//interpolate between them
			if (start && end)
			{
				float scale = max(0.f, glm::min(1.f, (frameTime - startTime) / (endTime - startTime)));

				//translation
				vec3 t = glm::mix(start->m_translation, end->m_translation, scale);

				//scale
				vec3 s = glm::mix(start->m_scale, end->m_scale, scale);

				//rotation (quaternion slerp)
				glm::quat r = glm::normalize(glm::slerp(start->m_rotation, end->m_rotation, scale));

				//build matrix
				mat4 rotMatrix = mat4_cast(r);
				mat4 scaleMatrix = glm::scale(s);
				mat4 translateMatrix = glm::translate(t);
				m_nodes[track->m_boneIndex]->m_localTransform = translateMatrix * scaleMatrix * rotMatrix;
			}
		}
	}
}

void FBXSkeleton::UpdateBones()
{
	//update bones
	for (int i = 0; i < (int)m_boneCount; ++i)
	{
		if (m_parentIndex[i] == -1)
			m_bones[i] = m_nodes[i]->m_localTransform;
		else
			m_bones[i] = m_bones[m_parentIndex[i]] * m_nodes[i]->m_localTransform;
	}
	//combine bind pose
	for (int i = 0; i < (int)m_boneCount; ++i)
		m_bones[i] = m_bones[i] * m_bindPoses[i];
}

void FBXFile::CalculateTangentsBinormals(vector<FBXVertex>& vertices, const vector<unsigned int>& indices)
{
	unsigned int vertexCount = (unsigned int)vertices.size();
	vec3* tan1 = new vec3[vertexCount * 2];
	vec3* tan2 = tan1 + vertexCount;
	memset(tan1, 0, vertexCount * sizeof(vec3) * 2);

	unsigned int indexCount = (unsigned int)indices.size();
	for (unsigned int a = 0; a < indexCount; a += 3)
	{
		unsigned int i1 = indices[a];
		unsigned int i2 = indices[a + 1];
		unsigned int i3 = indices[a + 2];

		const vec4& v1 = vertices[i1].m_position;
		const vec4& v2 = vertices[i2].m_position;
		const vec4& v3 = vertices[i3].m_position;

		const vec2& w1 = vertices[i1].m_texCoord1;
		const vec2& w2 = vertices[i2].m_texCoord1;
		const vec2& w3 = vertices[i3].m_texCoord1;

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

		float t = s1 * t2 - s2 * t1;
		float r = t == 0 ? 0 : 1.f / t;
		vec3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		vec3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		tan1[i1] += sdir;
		tan1[i2] += sdir;
		tan1[i3] += sdir;

		tan2[i1] += tdir;
		tan2[i2] += tdir;
		tan2[i3] += tdir;
	}

	for (unsigned int a = 0; a < vertexCount; a++)
	{
		const vec3& n = vec3(vertices[a].m_normal);
		const vec3& t = tan1[a];

		//Gram-Schmidt orthogonalise
		vec3 p = t - n * glm::dot(n, t);
		if (glm::length2(p))
		{
			vertices[a].m_tangent = vec4(glm::normalize(p), 0.f);

			//calculate binormal
			float sign = glm::dot(glm::cross(vec3(n), vec3(t)), vec3(tan2[a])) < 0.f ? -1.f : 1.f;
			vertices[a].m_binormal = vec4(glm::cross(vec3(vertices[a].m_normal), vec3(vertices[a].m_tangent)) * sign, 0);
		}
	}

	delete[] tan1;
}

unsigned int FBXFile::NodeCount(FBXNode* node)
{
	if (!node)
		return 0;

	unsigned int count = 1;

	for (auto n : node->m_children)
		count += NodeCount(n);

	return count;
}

FBXMeshNode* FBXFile::GetMeshByName(const char* name)
{
	for (auto mesh : m_meshes)
		if (mesh->m_name == name) return mesh;
	return nullptr;
}

FBXLightNode* FBXFile::GetLightByName(const char* name)
{
	auto iter = m_lights.find(name);
	if (iter != m_lights.end())
		return iter->second;
	return nullptr;
}

FBXCameraNode* FBXFile::GetCameraByName(const char* name)
{
	auto iter = m_cameras.find(name);
	if (iter != m_cameras.end())
		return iter->second;
	return nullptr;
}

FBXMaterial* FBXFile::GetMaterialByName(const char* name)
{
	auto iter = m_materials.find(name);
	if (iter != m_materials.end())
		return iter->second;
	return nullptr;
}

FBXTexture* FBXFile::GetTextureByName(const char* name)
{
	auto iter = m_textures.find(name);
	if (iter != m_textures.end())
		return iter->second;
	return nullptr;
}

FBXAnimation* FBXFile::GetAnimationByName(const char* name)
{
	auto iter = m_animations.find(name);
	if (iter != m_animations.end())
		return iter->second;
	return nullptr;
}

FBXLightNode* FBXFile::GetLightByIndex(unsigned int index)
{
	for (auto t : m_lights)
	{
		if (!index--)
			return t.second;
	}

	return nullptr;
}

FBXCameraNode* FBXFile::GetCameraByIndex(unsigned int index)
{
	for (auto t : m_cameras)
	{
		if (!index--)
			return t.second;
	}

	return nullptr;
}

FBXMaterial* FBXFile::GetMaterialByIndex(unsigned int index)
{
	for (auto t : m_materials)
	{
		if (!index--)
			return t.second;
	}

	return nullptr;
}

FBXAnimation* FBXFile::GetAnimationByIndex(unsigned int index)
{
	for (auto t : m_animations)
	{
		if (!index--)
			return t.second;
	}

	return nullptr;
}

FBXTexture* FBXFile::GetTextureByIndex(unsigned int index)
{
	for (auto t : m_textures)
	{
		if (!index--)
			return t.second;
	}

	return nullptr;
}

void FBXFile::GatherBones(void* object)
{
	FbxNode* fbxNode = (FbxNode*)object;

	if (fbxNode->GetNodeAttribute() && fbxNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		unsigned int index = (unsigned int)m_importAssistor->boneIndexList.size();
		m_importAssistor->boneIndexList[fbxNode->GetName()] = index;
	}

	for (int i = 0; i < fbxNode->GetChildCount(); i++)
		GatherBones((void*)fbxNode->GetChild(i));
}

void FBXNode::UpdateGlobalTransform()
{
	if (m_parent)
		m_globalTransform = m_parent->m_globalTransform * m_localTransform;
	else
		m_globalTransform = m_localTransform;

	for (auto child : m_children)
		child->UpdateGlobalTransform();
}

void FBXCameraNode::UpdateGlobalTransform()
{
	if (m_parent)
		m_globalTransform = m_parent->m_globalTransform * m_localTransform;
	else
		m_globalTransform = m_localTransform;

	m_viewMatrix = glm::inverse(m_globalTransform);

	for (auto child : m_children)
		child->UpdateGlobalTransform();
}

FBXTexture::~FBXTexture()
{
	delete[] data;
	glDeleteTextures(1, &handle);
}

FBXAnimation* FBXAnimation::Clone() const
{
	FBXAnimation* copy = new FBXAnimation();
	copy->m_name = m_name;
	copy->m_startFrame = m_startFrame;
	copy->m_endFrame = m_endFrame;
	copy->m_trackCount = m_trackCount;
	copy->m_tracks = new FBXTrack[m_trackCount];

	for (unsigned int i = 0; i < m_trackCount; ++i)
	{
		copy->m_tracks[i].m_boneIndex = m_tracks[i].m_boneIndex;
		copy->m_tracks[i].m_keyframeCount = m_tracks[i].m_keyframeCount;
		copy->m_tracks[i].m_keyframes = new FBXKeyFrame[m_tracks[i].m_keyframeCount];

		memcpy(copy->m_tracks[i].m_keyframes, m_tracks[i].m_keyframes, sizeof(FBXKeyFrame) * m_tracks[i].m_keyframeCount);
	}

	return copy;
}