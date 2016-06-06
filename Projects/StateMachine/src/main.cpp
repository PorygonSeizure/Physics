#include "StateMachineApp.h"

int main()
{	
	BaseApp* app = new StateMachineApp();
	if (app->Startup())
		app->Run();
	app->Shutdown();

	delete app;
	return 0;
}