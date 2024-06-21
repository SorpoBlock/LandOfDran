#pragma once

#include "../LandOfDran.h"

/*
	Contains possible flags and data parsed from command line arguments
	Will be expanded in the future
*/
struct ExecutableArguments
{
	//False if the entire program should shut down
	//Not loaded from command line arguments, but frequently passed with them
	bool mainLoopRun = true;

	//Are we hosting from a headless no UI dedicated servver
	bool dedicated = false;

	//Are we forgoing attempting to place our server on the server list
	bool silentMode = false;

	//Parse flags and data from command line arguments on start-up
	ExecutableArguments(int argc, char** argv);
};

