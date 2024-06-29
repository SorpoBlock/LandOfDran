#pragma once

#include "../LandOfDran.h"

//This function handles the implementation of the schedule and cancel Lua functions

extern "C"
{
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

struct Schedule
{
	//What time in MS since program start should this be executed
	int timeToExecute = 0;
	//from lastScheduleID
	int scheduleID = 0;
	//Schedule function is variatic
	int numLuaArgs = 0;
	//To call
	std::string functionName;
};

class LuaScheduler
{
	static int scheduleFunction(lua_State* L);
	static int cancelFunction(lua_State* L);

	//Each schedule has a unique ID that increments starting from 0
	static int lastScheduleID;

	static std::vector<Schedule> schedules;

	//These are schedules which were added from Lua being itself run by a schedule
	//So as to prevent an infinite loop, they aren't folded into the main vector until the end of the frame
	static std::vector<Schedule> schedulesThisFrame;

	//Likewise we can't cancel(id) within a schedule, we wait until the next frame minimum always
	static std::vector<unsigned int> tmpScheduleIDsToDelete;
	
	//If true, add new schedules to schedulesThisFrame until we're done running this frames schedules
	static bool runningSchedules;

public:
	LuaScheduler(lua_State* L);
	~LuaScheduler();

	//Checks for scheduled functions and runs them if needed, also handles schedule and cancel calls within scheduled functions
	void run(lua_State* L);
};

