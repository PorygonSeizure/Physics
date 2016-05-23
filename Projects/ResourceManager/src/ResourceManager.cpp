#include "Catch.hpp"
#include "ResourceManager.h"

class Texture
{
public:
	Texture(const char* filename) {}
};

class Font
{
public:
	Font(const char* filename, int fontSize) {}
};

TEST_CASE("TEST 1")
{
	Texture* resource1 = ResourceManager<Texture>::Load("image0.png");
	Texture* resource2 = ResourceManager<Texture>::Load("image0.png");
	Texture* resource3 = ResourceManager<Texture>::Load("image1.png");

	REQUIRE(resource1 != nullptr);
	REQUIRE(resource2 != nullptr);
	REQUIRE(resource1 == resource2);
	REQUIRE(resource3 != resource2);
	REQUIRE(ResourceManager<Texture>::CountLoadedResources() == 2);

	ResourceManager<Texture>::FreeAll();
	REQUIRE(ResourceManager<Texture>::CountLoadedResources() == 0);
}

//32 minutes