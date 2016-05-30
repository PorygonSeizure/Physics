//
// Copyright 2012-2016, Syoyo Fujita.
//
// Licensed under 2-clause BSD license.
//

//
// version 0.9.22: Introduce `LoadFlagsT`.
// version 0.9.20: Fixes creating per-face material using `usemtl`(#68)
// version 0.9.17: Support n-polygon and crease tag(OpenSubdiv extension)
// version 0.9.16: Make tinyobjloader header-only
// version 0.9.15: Change API to handle no mtl file case correctly(#58)
// version 0.9.14: Support specular highlight, bump, displacement and alpha
// map(#53)
// version 0.9.13: Report "Material file not found message" in `err`(#46)
// version 0.9.12: Fix groups being ignored if they have 'usemtl' just before
// 'g' (#44)
// version 0.9.11: Invert `Tr` parameter(#43)
// version 0.9.10: Fix seg fault on windows.
// version 0.9.9 : Replace atof() with custom parser.
// version 0.9.8 : Fix multi-materials(per-face material ID).
// version 0.9.7 : Support multi-materials(per-face material ID) per
// object/group.
// version 0.9.6 : Support Ni(index of refraction) mtl parameter.
//                 Parse transmittance material parameter correctly.
// version 0.9.5 : Parse multiple group name.
//                 Add support of specifying the base path to load material
//                 file.
// version 0.9.4 : Initial support of group tag(g)
// version 0.9.3 : Fix parsing triple 'x/y/z'
// version 0.9.2 : Add more .mtl load support
// version 0.9.1 : Add initial .mtl load support
// version 0.9.0 : Initial
//

//
// Use this in *one* .cc
//   #define TINYOBJLOADER_IMPLEMENTATION
//   #include "tiny_obj_loader.h"
//

#ifndef TINY_OBJ_LOADER_H_
#define TINY_OBJ_LOADER_H_

#include <string>
#include <vector>
#include <map>
#include <cmath>

namespace tinyobj
{

typedef struct MaterialT
{
	std::string name;

	float ambient[3];
	float diffuse[3];
	float specular[3];
	float transmittance[3];
	float emission[3];
	float shininess;
	float ior;	//index of refraction
	float dissolve;	//1 == opaque; 0 == fully transparent
	//illumination model (see http://www.fileformat.info/format/material/)
	int illum;

	int dummy;	//Suppress padding warning.

	std::string ambientTexname;	//map_Ka
	std::string diffuseTexname;	//map_Kd
	std::string specularTexname;	//map_Ks
	std::string specularHighlightTexname;	//map_Ns
	std::string bumpTexname;	//map_bump, bump
	std::string displacementTexname;	//disp
	std::string alphaTexname;	//map_d
	std::map<std::string, std::string> unknownParameter;
};

typedef struct TagT
{
	std::string name;

	std::vector<int> intValues;
	std::vector<float> floatValues;
	std::vector<std::string> stringValues;
};

typedef struct MeshT
{
	std::vector<float> positions;
	std::vector<float> normals;
	std::vector<float> texcoords;
	std::vector<unsigned int> indices;
	std::vector<unsigned char> numVertices;	// The number of vertices per face. Up to 255.
	std::vector<int> materialIDs;	// per-face material ID
	std::vector<TagT> tags;	 // SubD tag
};

typedef struct ShapeT
{
	std::string name;
	MeshT mesh;
};

typedef enum LoadFlagsT
{
	triangulation = 1,	// used whether triangulate polygon face in .obj
	calculateNormals = 2,	// used whether calculate the normals if the .obj normals are empty
	// Some nice stuff here
};

class Float3
{
public:
	Float3() : x(0.f), y(0.f), z(0.f) {}

	Float3(float coordX, float coordY, float coordZ) : x(coordX), y(coordY), z(coordZ) {}

	Float3(const Float3& from, const Float3& to)
	{
		coord[0] = to.coord[0] - from.coord[0];
		coord[1] = to.coord[1] - from.coord[1];
		coord[2] = to.coord[2] - from.coord[2];
	}

	Float3 CrossProduct(const Float3 & vec)
	{
		float a = y * vec.z - z * vec.y;
		float b = z * vec.x - x * vec.z;
		float c = x * vec.y - y * vec.x;
		return Float3(a, b, c);
	}

	void Normalize()
	{
		const float length = std::sqrt((coord[0] * coord[0]) + (coord[1] * coord[1]) + (coord[2] * coord[2]));
		if (length != 1)
		{
			coord[0] = (coord[0] / length);
			coord[1] = (coord[1] / length);
			coord[2] = (coord[2] / length);
		}
	}

private:
	union
	{
		float coord[3];
		struct
		{
			float x;
			float y;
			float z;
		};
	};
};

class MaterialReader
{
public:
	MaterialReader() {}
	virtual ~MaterialReader();

	virtual bool operator()(const std::string &matId, std::vector<MaterialT> &materials, std::map<std::string, int> &matMap, std::string &err) = 0;
};

class MaterialFileReader : public MaterialReader
{
public:
	MaterialFileReader(const std::string &mtlBasepath) : m_mtlBasePath(mtlBasepath) {}
	virtual ~MaterialFileReader() {}
	virtual bool operator()(const std::string &matId, std::vector<MaterialT> &materials, std::map<std::string, int> &matMap, std::string &err);

private:
	std::string m_mtlBasePath;
};

///Loads .obj from a file.
///'shapes' will be filled with parsed shape data
///The function returns error string.
///Returns true when loading .obj become success.
///Returns warning and error message into `err`
///'mtlBasepath' is optional, and used for base path for .mtl file.
///'optional flags
bool LoadObj(std::vector<ShapeT>& shapes /*[output]*/,	std::vector<MaterialT>& materials /*[output]*/, std::string& err /*[output]*/, const char* filename, const char* mtlBasepath = NULL, 
				unsigned int flags = 1);

///Loads object from a std::istream, uses GetMtlIStreamFn to retrieve
///std::istream for materials.
///Returns true when loading .obj become success.
///Returns warning and error message into `err`
bool LoadObj(std::vector<ShapeT>& shapes /*[output]*/, std::vector<MaterialT>& materials /*[output]*/, std::string& err /*[output]*/, std::istream& inStream, MaterialReader& readMatFn, 
				unsigned int flags = 1);

///Loads materials into std::map
void LoadMtl(std::map<std::string, int>& materialMap /*[output]*/, std::vector<MaterialT>& materials /*[output]*/, std::istream &inStream);
}

#ifdef TINYOBJLOADER_IMPLEMENTATION
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cctype>

#include <fstream>
#include <sstream>

#include "tinyOBJLoader.h"

namespace tinyobj
{

MaterialReader::~MaterialReader() {}

#define TINYOBJ_SSCANF_BUFFER_SIZE (4096)

struct VertexIndex
{
	int vIdx;
	int vtIdx;
	int vnIdx;
	VertexIndex() : vIdx(-1), vtIdx(-1), vnIdx(-1) {}
	explicit VertexIndex(int idx) : vIdx(idx), vtIdx(idx), vnIdx(idx) {}
	VertexIndex(int vidx, int vtidx, int vnidx) : vIdx(vidx), vtIdx(vtidx), vnIdx(vnidx) {}
};

struct TagSizes
{
	TagSizes() : numInts(0), numFloats(0), numStrings(0) {}
	int numInts;
	int numFloats;
	int numStrings;
};

// for std::map
static inline bool operator<(const VertexIndex &a, const VertexIndex &b)
{
	if (a.vIdx != b.vIdx)
		return (a.vIdx < b.vIdx);
	if (a.vnIdx != b.vnIdx)
		return (a.vnIdx < b.vnIdx);
	if (a.vtIdx != b.vtIdx)
		return (a.vtIdx < b.vtIdx);

	return false;
}

struct objShape
{
	std::vector<float> v;
	std::vector<float> vn;
	std::vector<float> vt;
};

//See http://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf
std::istream& SafeGetline(std::istream& is, std::string& t)
{
	t.clear();

	//The characters in the stream are read one-by-one using a std::streambuf.
	//That is faster than reading them one-by-one using the std::istream.
	//Code that uses streambuf this way must be guarded by a sentry object.
	//The sentry object performs various tasks,
	//such as thread synchronization and updating the stream state.

	std::istream::sentry se(is, true);
	std::streambuf* sb = is.rdbuf();

	for (;;)
	{
		int c = sb->sbumpc();
		switch (c)
		{
		case '\n':
			return is;
		case '\r':
			if (sb->sgetc() == '\n')
				sb->sbumpc();
			return is;
		case EOF:
			//Also handle the case when the last line has no line ending
			if (t.empty())
				is.setstate(std::ios::eofbit);
			return is;
		default:
			t += (char)c;
		}
	}
}

#define IS_SPACE(x) (((x) == ' ') || ((x) == '\t'))
#define IS_DIGIT(x) ((unsigned int)((x) - '0') < (unsigned int)10)
#define IS_NEW_LINE(x) (((x) == '\r') || ((x) == '\n') || ((x) == '\0'))

//Make index zero-base, and also support relative index.
static inline int FixIndex(int idx, int n)
{
	if (idx > 0)
		return idx - 1;
	if (idx == 0)
		return 0;
	return n + idx;	// negative value = relative
}

static inline std::string ParseString(const char*& token)
{
	std::string s;
	token += strspn(token, " \t");
	size_t e = strcspn(token, " \t\r");
	s = std::string(token, &token[e]);
	token += e;
	return s;
}

static inline int ParseInt(const char*& token)
{
	token += strspn(token, " \t");
	int i = atoi(token);
	token += strcspn(token, " \t\r");
	return i;
}

//Tries to parse a floating point number located at s.
//
//sEnd should be a location in the string where reading should absolutely
//stop. For example at the end of the string, to prevent buffer overflows.
//
//Parses the following EBNF grammar:
//  sign    = "+" | "-" ;
//  END     = ? anything not in digit ?
//  digit   = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
//  integer = [sign] , digit , {digit} ;
//  decimal = integer , ["." , integer] ;
//  float   = (decimal , END) | (decimal , ("E" | "e") , integer , END) ;
//
// Valid strings are for example:
//  -0	 +3.1417e+2  -0.0E-3  1.0324  -1.41   11e2
//
//If the parsing is a success, result is set to the parsed value and true
//is returned.
//
//The function is greedy and will parse until any of the following happens:
// - a non-conforming character is encountered.
// - sEnd is reached.
//
//The following situations triggers a failure:
// - s >= sEnd.
// - parse failure.
//
static bool TryParseDouble(const char* s, const char* sEnd, double* result)
{
	if (s >= sEnd)
		return false;

	double mantissa = 0.0;
	// This exponent is base 2 rather than 10.
	// However the exponent we parse is supposed to be one of ten,
	// thus we must take care to convert the exponent/and or the
	// mantissa to a * 2^E, where a is the mantissa and E is the
	// exponent.
	// To get the final double we will use ldexp, it requires the
	// exponent to be in base 2.
	int exponent = 0;

	// NOTE: THESE MUST BE DECLARED HERE SINCE WE ARE NOT ALLOWED
	// TO JUMP OVER DEFINITIONS.
	char sign = '+';
	char expSign = '+';
	char const* curr = s;

	// How many characters were read in a loop.
	int read = 0;
	// Tells whether a loop terminated due to reaching sEnd.
	bool endNotReached = false;

	/*
	BEGIN PARSING.
	*/

	// Find out what sign we've got.
	if (*curr == '+' || *curr == '-')
	{
		sign = *curr;
		curr++;
	}
	else if (IS_DIGIT(*curr)) { /*Pass through.*/ }
	else
		goto fail;

	// Read the integer part.
	while ((endNotReached = (curr != sEnd)) && IS_DIGIT(*curr))
	{
		mantissa *= 10;
		mantissa += static_cast<int>(*curr - 0x30);
		curr++;
		read++;
	}

	// We must make sure we actually got something.
	if (read == 0)
		goto fail;
	// We allow numbers of form "#", "###" etc.
	if (!endNotReached)
		goto assemble;

	// Read the decimal part.
	if (*curr == '.')
	{
		curr++;
		read = 1;
		while ((endNotReached = (curr != sEnd)) && IS_DIGIT(*curr))
		{
			// NOTE: Don't use powf here, it will absolutely murder precision.
			mantissa += static_cast<int>(*curr - 0x30) * pow(10.0, -read);
			read++;
			curr++;
		}
	}
	else if (*curr == 'e' || *curr == 'E') {}
	else { goto assemble; }

	if (!endNotReached)
		goto assemble;

	// Read the exponent part.
	if (*curr == 'e' || *curr == 'E')
	{
		curr++;
		// Figure out if a sign is present and if it is.
		if ((endNotReached = (curr != sEnd)) && (*curr == '+' || *curr == '-'))
		{
			expSign = *curr;
			curr++;
		}
		else if (IS_DIGIT(*curr)) { /*Pass through.*/ }
		else { goto fail; }	// Empty E is not allowed.

		read = 0;
		while ((endNotReached = (curr != sEnd)) && IS_DIGIT(*curr))
		{
			exponent *= 10;
			exponent += static_cast<int>(*curr - 0x30);
			curr++;
			read++;
		}
		exponent *= (expSign == '+' ? 1 : -1);
		if (read == 0)
			goto fail;
	}

assemble:
	*result = (sign == '+' ? 1 : -1) * ldexp(mantissa * pow(5.0, exponent), exponent);
	return true;
fail:
	return false;
}
static inline float ParseFloat(const char*& token)
{
	token += strspn(token, " \t");
#ifdef TINY_OBJ_LOADER_OLD_FLOAT_PARSER
	float f = (float)atof(token);
	token += strcspn(token, " \t\r");
#else
	const char *end = token + strcspn(token, " \t\r");
	double val = 0.0;
	TryParseDouble(token, end, &val);
	float f = static_cast<float>(val);
	token = end;
#endif
	return f;
}

static inline void ParseFloat2(float& x, float& y, const char*& token)
{
	x = ParseFloat(token);
	y = ParseFloat(token);
}

static inline void ParseFloat3(float& x, float& y, float& z, const char*& token)
{
	x = ParseFloat(token);
	y = ParseFloat(token);
	z = ParseFloat(token);
}

static TagSizes ParseTagTriple(const char*& token)
{
	TagSizes ts;

	ts.numInts = atoi(token);
	token += strcspn(token, "/ \t\r");
	if (token[0] != '/')
		return ts;
	token++;

	ts.numFloats = atoi(token);
	token += strcspn(token, "/ \t\r");
	if (token[0] != '/')
		return ts;
	token++;

	ts.numStrings = atoi(token);
	token += strcspn(token, "/ \t\r") + 1;

	return ts;
}

//Parse triples: i, i/j/k, i//k, i/j
static VertexIndex ParseTriple(const char*& token, int vsize, int vnsize, int vtsize)
{
	VertexIndex vi(-1);

	vi.vIdx = FixIndex(atoi(token), vsize);
	token += strcspn(token, "/ \t\r");
	if (token[0] != '/')
		return vi;
	token++;

	//i//k
	if (token[0] == '/')
	{
		token++;
		vi.vnIdx = FixIndex(atoi(token), vnsize);
		token += strcspn(token, "/ \t\r");
		return vi;
	}

	//i/j/k or i/j
	vi.vtIdx = FixIndex(atoi(token), vtsize);
	token += strcspn(token, "/ \t\r");
	if (token[0] != '/')
		return vi;

	//i/j/k
	token++;	//skip '/'
	vi.vnIdx = FixIndex(atoi(token), vnsize);
	token += strcspn(token, "/ \t\r");
	return vi;
}

static unsigned int UpdateVertex(std::map<VertexIndex, unsigned int>& vertexCache, std::vector<float>& positions, std::vector<float>& normals, std::vector<float>& texcoords, 
									const std::vector<float>& inPositions, const std::vector<float>& inNormals, const std::vector<float>& inTexcoords, const VertexIndex& i)
{
	const std::map<VertexIndex, unsigned int>::iterator it = vertexCache.find(i);

	if (it != vertexCache.end())
		return it->second;	//found cache

	assert(inPositions.size() > static_cast<unsigned int>(3 * i.vIdx + 2));

	positions.push_back(inPositions[3 * static_cast<size_t>(i.vIdx) + 0]);
	positions.push_back(inPositions[3 * static_cast<size_t>(i.vIdx) + 1]);
	positions.push_back(inPositions[3 * static_cast<size_t>(i.vIdx) + 2]);

	if ((i.vnIdx >= 0) && (static_cast<size_t>(i.vnIdx * 3 + 2) < inNormals.size()))
	{
		normals.push_back(inNormals[3 * static_cast<size_t>(i.vnIdx) + 0]);
		normals.push_back(inNormals[3 * static_cast<size_t>(i.vnIdx) + 1]);
		normals.push_back(inNormals[3 * static_cast<size_t>(i.vnIdx) + 2]);
	}

	if ((i.vtIdx >= 0) && (static_cast<size_t>(i.vtIdx * 2 + 1) < inTexcoords.size()))
	{
		texcoords.push_back(inTexcoords[2 * static_cast<size_t>(i.vtIdx) + 0]);
		texcoords.push_back(inTexcoords[2 * static_cast<size_t>(i.vtIdx) + 1]);
	}

	unsigned int idx = static_cast<unsigned int>(positions.size() / 3 - 1);
	vertexCache[i] = idx;

	return idx;
}

static void InitMaterial(MaterialT& material)
{
	material.name = "";
	material.ambientTexname = "";
	material.diffuseTexname = "";
	material.specularTexname = "";
	material.specularHighlightTexname = "";
	material.bumpTexname = "";
	material.displacementTexname = "";
	material.alphaTexname = "";
	for (int i = 0; i < 3; i++)
	{
		material.ambient[i] = 0.f;
		material.diffuse[i] = 0.f;
		material.specular[i] = 0.f;
		material.transmittance[i] = 0.f;
		material.emission[i] = 0.f;
	}
	material.illum = 0;
	material.dissolve = 1.f;
	material.shininess = 1.f;
	material.ior = 1.f;
	material.unknownParameter.clear();
}

static bool ExportFaceGroupToShape(ShapeT& shape, std::map<VertexIndex, unsigned int> vertexCache, const std::vector<float>& inPositions, const std::vector<float>& inNormals, 
									const std::vector<float>& inTexcoords, const std::vector<std::vector<VertexIndex>>& faceGroup, std::vector<TagT>& tags, const int materialID, 
									const std::string& name, bool clearCache, unsigned int flags, std::string& err)
{
	if (faceGroup.empty())
		return false;

	bool triangulate((flags & triangulation) == triangulation);
	bool normalsCalculation((flags & calculateNormals) == calculateNormals);

	// Flatten vertices and indices
	for (size_t i = 0; i < faceGroup.size(); i++)
	{
		const std::vector<VertexIndex> &face = faceGroup[i];

		VertexIndex i0 = face[0];
		VertexIndex i1(-1);
		VertexIndex i2 = face[1];

		size_t npolys = face.size();

		if (triangulate)
		{
			// Polygon -> triangle fan conversion
			for (size_t k = 2; k < npolys; k++)
			{
				i1 = i2;
				i2 = face[k];

				unsigned int v0 = UpdateVertex(vertexCache, shape.mesh.positions, shape.mesh.normals, shape.mesh.texcoords, inPositions, inNormals, inTexcoords, i0);
				unsigned int v1 = UpdateVertex(vertexCache, shape.mesh.positions, shape.mesh.normals, shape.mesh.texcoords, inPositions, inNormals, inTexcoords, i1);
				unsigned int v2 = UpdateVertex(vertexCache, shape.mesh.positions, shape.mesh.normals, shape.mesh.texcoords, inPositions, inNormals, inTexcoords, i2);

				shape.mesh.indices.push_back(v0);
				shape.mesh.indices.push_back(v1);
				shape.mesh.indices.push_back(v2);

				shape.mesh.numVertices.push_back(3);
				shape.mesh.materialIDs.push_back(materialID);
			}
		}
		else
		{
			for (size_t k = 0; k < npolys; k++)
			{
				unsigned int v = UpdateVertex(vertexCache, shape.mesh.positions, shape.mesh.normals, shape.mesh.texcoords, inPositions, inNormals, inTexcoords, face[k]);

				shape.mesh.indices.push_back(v);
			}

			shape.mesh.numVertices.push_back(static_cast<unsigned char>(npolys));
			shape.mesh.materialIDs.push_back(materialID);	// per face
		}
	}

	if (normalsCalculation && shape.mesh.normals.empty())
	{
		const size_t nIndexs = shape.mesh.indices.size();
		if (nIndexs % 3 == 0)
		{
			shape.mesh.normals.resize(shape.mesh.positions.size());
			for (register size_t iIndices = 0; iIndices < nIndexs; iIndices += 3)
			{
				Float3 v1;
				Float3 v2;
				Float3 v3;
				memcpy(&v1, &shape.mesh.positions[shape.mesh.indices[iIndices] * 3], sizeof(Float3));
				memcpy(&v2, &shape.mesh.positions[shape.mesh.indices[iIndices + 1] * 3], sizeof(Float3));
				memcpy(&v3, &shape.mesh.positions[shape.mesh.indices[iIndices + 2] * 3], sizeof(Float3));

				Float3 v12(v1, v2);
				Float3 v13(v1, v3);

				Float3 normal = v12.CrossProduct(v13);
				normal.Normalize();

				memcpy(&shape.mesh.normals[shape.mesh.indices[iIndices] * 3], &normal, sizeof(Float3));
				memcpy(&shape.mesh.normals[shape.mesh.indices[iIndices + 1] * 3], &normal, sizeof(Float3));
				memcpy(&shape.mesh.normals[shape.mesh.indices[iIndices + 2] * 3], &normal, sizeof(Float3));
			}
		}
		else
		{
			std::stringstream ss;
			ss << "WARN: The shape " << name << 
				" does not have a topology of triangles, therfore the normals calculation could not be performed. Select the tinyobj::triangulation flag for this object." << std::endl;
			err += ss.str();
		}
	}

	shape.name = name;
	shape.mesh.tags.swap(tags);

	if (clearCache)
		vertexCache.clear();

	return true;
}

void LoadMtl(std::map<std::string, int>& materialMap, std::vector<MaterialT>& materials, std::istream& inStream)
{
	//Create a default material anyway.
	MaterialT material;
	InitMaterial(material);

	while (inStream.peek() != -1)
	{
		std::string linebuf;
		SafeGetline(inStream, linebuf);

		//Trim newline '\r\n' or '\n'
		if (linebuf.size() > 0)
		{
			if (linebuf[linebuf.size() - 1] == '\n')
				linebuf.erase(linebuf.size() - 1);
		}
		if (linebuf.size() > 0)
		{
			if (linebuf[linebuf.size() - 1] == '\r')
				linebuf.erase(linebuf.size() - 1);
		}

		//Skip if empty line.
		if (linebuf.empty())
			continue;

		//Skip leading space.
		const char* token = linebuf.c_str();
		token += strspn(token, " \t");

		assert(token);
		if (token[0] == '\0')
			continue;	//empty line

		if (token[0] == '#')
			continue;	//comment line

		//new mtl
		if ((0 == strncmp(token, "newmtl", 6)) && IS_SPACE((token[6])))
		{
			//flush previous material.
			if (!material.name.empty())
			{
				materialMap.insert(std::pair<std::string, int>(material.name, static_cast<int>(materials.size())));
				materials.push_back(material);
			}

			//initial temporary material
			InitMaterial(material);

			//set new mtl name
			char namebuf[TINYOBJ_SSCANF_BUFFER_SIZE];
			token += 7;
#ifdef _MSC_VER
			sscanf_s(token, "%s", namebuf, (unsigned)_countof(namebuf));
#else
			sscanf(token, "%s", namebuf);
#endif
			material.name = namebuf;
			continue;
		}

		//ambient
		if (token[0] == 'K' && token[1] == 'a' && IS_SPACE((token[2])))
		{
			token += 2;
			float r;
			float g;
			float b;
			ParseFloat3(r, g, b, token);
			material.ambient[0] = r;
			material.ambient[1] = g;
			material.ambient[2] = b;
			continue;
		}

		//diffuse
		if (token[0] == 'K' && token[1] == 'd' && IS_SPACE((token[2])))
		{
			token += 2;
			float r;
			float g;
			float b;
			ParseFloat3(r, g, b, token);
			material.diffuse[0] = r;
			material.diffuse[1] = g;
			material.diffuse[2] = b;
			continue;
		}

		//specular
		if (token[0] == 'K' && token[1] == 's' && IS_SPACE((token[2])))
		{
			token += 2;
			float r;
			float g;
			float b;
			ParseFloat3(r, g, b, token);
			material.specular[0] = r;
			material.specular[1] = g;
			material.specular[2] = b;
			continue;
		}

		//transmittance
		if (token[0] == 'K' && token[1] == 't' && IS_SPACE((token[2])))
		{
			token += 2;
			float r;
			float g;
			float b;
			ParseFloat3(r, g, b, token);
			material.transmittance[0] = r;
			material.transmittance[1] = g;
			material.transmittance[2] = b;
			continue;
		}

		//ior(index of refraction)
		if (token[0] == 'N' && token[1] == 'i' && IS_SPACE((token[2])))
		{
			token += 2;
			material.ior = ParseFloat(token);
			continue;
		}

		//emission
		if (token[0] == 'K' && token[1] == 'e' && IS_SPACE(token[2]))
		{
			token += 2;
			float r;
			float g;
			float b;
			ParseFloat3(r, g, b, token);
			material.emission[0] = r;
			material.emission[1] = g;
			material.emission[2] = b;
			continue;
		}

		//shininess
		if (token[0] == 'N' && token[1] == 's' && IS_SPACE(token[2]))
		{
			token += 2;
			material.shininess = ParseFloat(token);
			continue;
		}

		//illum model
		if (0 == strncmp(token, "illum", 5) && IS_SPACE(token[5]))
		{
			token += 6;
			material.illum = ParseInt(token);
			continue;
		}

		//dissolve
		if ((token[0] == 'd' && IS_SPACE(token[1])))
		{
			token += 1;
			material.dissolve = ParseFloat(token);
			continue;
		}
		if (token[0] == 'T' && token[1] == 'r' && IS_SPACE(token[2]))
		{
			token += 2;
			//Invert value of Tr(assume Tr is in range [0, 1])
			material.dissolve = 1.f - ParseFloat(token);
			continue;
		}

		//ambient texture
		if ((0 == strncmp(token, "map_Ka", 6)) && IS_SPACE(token[6]))
		{
			token += 7;
			material.ambientTexname = token;
			continue;
		}

		//diffuse texture
		if ((0 == strncmp(token, "map_Kd", 6)) && IS_SPACE(token[6]))
		{
			token += 7;
			material.diffuseTexname = token;
			continue;
		}

		//specular texture
		if ((0 == strncmp(token, "map_Ks", 6)) && IS_SPACE(token[6]))
		{
			token += 7;
			material.specularTexname = token;
			continue;
		}

		//specular highlight texture
		if ((0 == strncmp(token, "map_Ns", 6)) && IS_SPACE(token[6]))
		{
			token += 7;
			material.specularHighlightTexname = token;
			continue;
		}

		//bump texture
		if ((0 == strncmp(token, "map_bump", 8)) && IS_SPACE(token[8]))
		{
			token += 9;
			material.bumpTexname = token;
			continue;
		}

		//alpha texture
		if ((0 == strncmp(token, "map_d", 5)) && IS_SPACE(token[5]))
		{
			token += 6;
			material.alphaTexname = token;
			continue;
		}

		//bump texture
		if ((0 == strncmp(token, "bump", 4)) && IS_SPACE(token[4]))
		{
			token += 5;
			material.bumpTexname = token;
			continue;
		}

		//displacement texture
		if ((0 == strncmp(token, "disp", 4)) && IS_SPACE(token[4]))
		{
			token += 5;
			material.displacementTexname = token;
			continue;
		}

		//unknown parameter
		const char* _space = strchr(token, ' ');
		if (!_space)
			_space = strchr(token, '\t');
		if (_space)
		{
			std::ptrdiff_t len = _space - token;
			std::string key(token, static_cast<size_t>(len));
			std::string value = _space + 1;
			material.unknownParameter.insert(std::pair<std::string, std::string>(key, value));
		}
	}
	//flush last material.
	materialMap.insert(std::pair<std::string, int>(material.name, static_cast<int>(materials.size())));
	materials.push_back(material);
}

bool MaterialFileReader::operator()(const std::string& matId, std::vector<MaterialT>& materials, std::map<std::string, int>& matMap, std::string& err)
{
	std::string filepath;

	if (!m_mtlBasePath.empty())
		filepath = std::string(m_mtlBasePath) + matId;
	else
		filepath = matId;

	std::ifstream matIStream(filepath.c_str());
	LoadMtl(matMap, materials, matIStream);
	if (!matIStream)
	{
		std::stringstream ss;
		ss << "WARN: Material file [ " << filepath << " ] not found. Created a default material.";
		err += ss.str();
	}
	return true;
}

bool LoadObj(std::vector<ShapeT>& shapes /*[output]*/, std::vector<MaterialT>& materials /*[output]*/, std::string& err, const char* filename, const char* mtlBasepath, unsigned int flags)
{
	shapes.clear();

	std::stringstream errss;

	std::ifstream ifs(filename);
	if (!ifs)
	{
		errss << "Cannot open file [" << filename << "]" << std::endl;
		err = errss.str();
		return false;
	}

	std::string basePath;
	if (mtlBasepath)
		basePath = mtlBasepath;
	MaterialFileReader matFileReader(basePath);

	return LoadObj(shapes, materials, err, ifs, matFileReader, flags);
}

bool LoadObj(std::vector<ShapeT>& shapes /*[output]*/, std::vector<MaterialT>& materials /*[output]*/, std::string& err, std::istream& inStream, MaterialReader& readMatFn, unsigned int flags)
{
	std::stringstream errss;

	std::vector<float> v;
	std::vector<float> vn;
	std::vector<float> vt;
	std::vector<TagT> tags;
	std::vector<std::vector<VertexIndex>> faceGroup;
	std::string name;

	//material
	std::map<std::string, int> materialMap;
	std::map<VertexIndex, unsigned int> vertexCache;
	int material = -1;

	ShapeT shape;

	while (inStream.peek() != -1)
	{
		std::string linebuf;
		SafeGetline(inStream, linebuf);

		//Trim newline '\r\n' or '\n'
		if (linebuf.size() > 0)
		{
			if (linebuf[linebuf.size() - 1] == '\n')
				linebuf.erase(linebuf.size() - 1);
		}
		if (linebuf.size() > 0)
		{
			if (linebuf[linebuf.size() - 1] == '\r')
				linebuf.erase(linebuf.size() - 1);
		}

		// Skip if empty line.
		if (linebuf.empty())
			continue;

		// Skip leading space.
		const char* token = linebuf.c_str();
		token += strspn(token, " \t");

		assert(token);
		if (token[0] == '\0')
			continue;	//empty line

		if (token[0] == '#')
			continue;	//comment line

		//vertex
		if (token[0] == 'v' && IS_SPACE((token[1])))
		{
			token += 2;
			float x;
			float y;
			float z;
			ParseFloat3(x, y, z, token);
			v.push_back(x);
			v.push_back(y);
			v.push_back(z);
			continue;
		}

		//normal
		if (token[0] == 'v' && token[1] == 'n' && IS_SPACE((token[2])))
		{
			token += 3;
			float x;
			float y;
			float z;
			ParseFloat3(x, y, z, token);
			vn.push_back(x);
			vn.push_back(y);
			vn.push_back(z);
			continue;
		}

		//texcoord
		if (token[0] == 'v' && token[1] == 't' && IS_SPACE((token[2])))
		{
			token += 3;
			float x;
			float y;
			ParseFloat2(x, y, token);
			vt.push_back(x);
			vt.push_back(y);
			continue;
		}

		//face
		if (token[0] == 'f' && IS_SPACE((token[1])))
		{
			token += 2;
			token += strspn(token, " \t");

			std::vector<VertexIndex> face;
			face.reserve(3);

			while (!IS_NEW_LINE(token[0]))
			{
				VertexIndex vi = ParseTriple(token, static_cast<int>(v.size() / 3), static_cast<int>(vn.size() / 3), static_cast<int>(vt.size() / 2));
				face.push_back(vi);
				size_t n = strspn(token, " \t\r");
				token += n;
			}

			//replace with emplace_back + std::move on C++11
			faceGroup.push_back(std::vector<VertexIndex>());
			faceGroup[faceGroup.size() - 1].swap(face);

			continue;
		}

		//use mtl
		if ((0 == strncmp(token, "usemtl", 6)) && IS_SPACE((token[6])))
		{
			char namebuf[TINYOBJ_SSCANF_BUFFER_SIZE];
			token += 7;
#ifdef _MSC_VER
			sscanf_s(token, "%s", namebuf, (unsigned)_countof(namebuf));
#else
			sscanf(token, "%s", namebuf);
#endif

			int newMaterialId = -1;
			if (materialMap.find(namebuf) != materialMap.end())
				newMaterialId = materialMap[namebuf];
			else { /*{ error!! material not found }*/ }

			if (newMaterialId != material)
			{
				// Create per-face material
				ExportFaceGroupToShape(shape, vertexCache, v, vn, vt, faceGroup, tags, material, name, true, flags, err);
				faceGroup.clear();
				material = newMaterialId;
			}

			continue;
		}

		// load mtl
		if ((0 == strncmp(token, "mtllib", 6)) && IS_SPACE((token[6])))
		{
			char namebuf[TINYOBJ_SSCANF_BUFFER_SIZE];
			token += 7;
#ifdef _MSC_VER
			sscanf_s(token, "%s", namebuf, (unsigned)_countof(namebuf));
#else
			sscanf(token, "%s", namebuf);
#endif

			std::string errMtl;
			bool ok = readMatFn(namebuf, materials, materialMap, errMtl);
			err += errMtl;

			if (!ok)
			{
				faceGroup.clear();	//for safety
				return false;
			}

			continue;
		}

		//group name
		if (token[0] == 'g' && IS_SPACE((token[1])))
		{
			//flush previous face group.
			bool ret = ExportFaceGroupToShape(shape, vertexCache, v, vn, vt, faceGroup, tags, material, name, true, flags, err);
			if (ret)
				shapes.push_back(shape);

			shape = ShapeT();

			//material = -1;
			faceGroup.clear();

			std::vector<std::string> names;
			names.reserve(2);

			while (!IS_NEW_LINE(token[0]))
			{
				std::string str = ParseString(token);
				names.push_back(str);
				token += strspn(token, " \t\r");	//skip tag
			}

			assert(names.size() > 0);

			//names[0] must be 'g', so skip the 0th element.
			if (names.size() > 1)
				name = names[1];
			else
				name = "";

			continue;
		}

		//object name
		if (token[0] == 'o' && IS_SPACE((token[1])))
		{

			//flush previous face group.
			bool ret = ExportFaceGroupToShape(shape, vertexCache, v, vn, vt, faceGroup, tags, material, name, true, flags, err);
			if (ret)
				shapes.push_back(shape);

			//material = -1;
			faceGroup.clear();
			shape = ShapeT();

			//@todo { multiple object name? }
			char namebuf[TINYOBJ_SSCANF_BUFFER_SIZE];
			token += 2;
#ifdef _MSC_VER
			sscanf_s(token, "%s", namebuf, (unsigned)_countof(namebuf));
#else
			sscanf(token, "%s", namebuf);
#endif
			name = std::string(namebuf);

			continue;
		}

		if (token[0] == 't' && IS_SPACE(token[1]))
		{
			TagT tag;

			char namebuf[4096];
			token += 2;
#ifdef _MSC_VER
			sscanf_s(token, "%s", namebuf, (unsigned)_countof(namebuf));
#else
			sscanf(token, "%s", namebuf);
#endif
			tag.name = std::string(namebuf);

			token += tag.name.size() + 1;

			TagSizes ts = ParseTagTriple(token);

			tag.intValues.resize(static_cast<size_t>(ts.numInts));

			for (size_t i = 0; i < static_cast<size_t>(ts.numInts); ++i)
			{
				tag.intValues[i] = atoi(token);
				token += strcspn(token, "/ \t\r") + 1;
			}

			tag.floatValues.resize(static_cast<size_t>(ts.numFloats));
			for (size_t i = 0; i < static_cast<size_t>(ts.numFloats); ++i)
			{
				tag.floatValues[i] = ParseFloat(token);
				token += strcspn(token, "/ \t\r") + 1;
			}

			tag.stringValues.resize(static_cast<size_t>(ts.numStrings));
			for (size_t i = 0; i < static_cast<size_t>(ts.numStrings); ++i)
			{
				char stringValueBuffer[4096];

#ifdef _MSC_VER
				sscanf_s(token, "%s", stringValueBuffer, (unsigned)_countof(stringValueBuffer));
#else
				sscanf(token, "%s", stringValueBuffer);
#endif
				tag.stringValues[i] = stringValueBuffer;
				token += tag.stringValues[i].size() + 1;
			}

			tags.push_back(tag);
		}

		//Ignore unknown command.
	}

	bool ret = ExportFaceGroupToShape(shape, vertexCache, v, vn, vt, faceGroup, tags, material, name, true, flags, err);
	if (ret)
		shapes.push_back(shape);
	faceGroup.clear();	//for safety

	err += errss.str();

	return true;
}

}	//namespace

#endif

#endif	//TINY_OBJ_LOADER_H_