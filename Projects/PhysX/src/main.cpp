#include "PhysicsApp.h"
#include <GLFW/glfw3.h>

int main()
{
	BaseApp* app = new PhysicsApp();
	if (app->Startup())
		app->Run();
	app->Shutdown();

	return 0;
}