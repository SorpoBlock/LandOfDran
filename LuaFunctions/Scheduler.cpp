#include "Scheduler.h"

int LuaScheduler::lastScheduleID = 1;
std::vector<Schedule> LuaScheduler::schedules;
std::vector<Schedule> LuaScheduler::schedulesThisFrame;
bool LuaScheduler::runningSchedules = false;
std::vector<unsigned int> LuaScheduler::tmpScheduleIDsToDelete;

LuaScheduler::LuaScheduler(lua_State* L)
{
	lua_register(L, "schedule", LuaScheduler::scheduleFunction);
	lua_register(L, "cancel", LuaScheduler::cancelFunction);
}

LuaScheduler::~LuaScheduler()
{

}

void LuaScheduler::run(lua_State * L)
{
    runningSchedules = true;
    auto sch = schedules.begin();
    while (sch != schedules.end())
    {
        int args = lua_gettop(L);
        if (args > 0)
            lua_pop(L, args);

        if ((*sch).timeToExecute < SDL_GetTicks())
        {
            lua_getglobal(L, (*sch).functionName.c_str());
            if (!lua_isfunction(L, 1))
            {
                error("Schedule was passed nil or another type instead of a function! Name: " + (*sch).functionName);
                error("Error in schedule " + std::to_string((*sch).scheduleID));
                int args = lua_gettop(L);
                if (args > 0)
                    lua_pop(L, args);
            }
            else
            {
                int argsToPush = 0;

                if ((*sch).numLuaArgs > 0)
                {
                    lua_getglobal(L, "scheduleArgs");
                    if (!lua_istable(L, -1))
                    {
                        error("Where did your scheduleArgs table go!");
                        lua_pop(L, 1);
                    }
                    else
                    {
                        lua_pushinteger(L, (*sch).scheduleID);
                        lua_gettable(L, -2);

                        //func name
                        //scheduleArgs table
                        //our specific args table

                        if (lua_isnil(L, -1))
                            lua_pop(L, 2);
                        else
                        {
                            for (int g = (*sch).numLuaArgs - 1; g >= 0; g--)
                            {
                                lua_pushinteger(L, g + 1);
                                lua_gettable(L, 3);
                            }

                            //func name
                            //scheduleArgs table
                            //our specific args table
                            //arg1...
                            //arg2...

                            lua_pushinteger(L, (*sch).scheduleID);
                            lua_pushnil(L);

                            lua_settable(L, 2);

                            lua_remove(L, 3);
                            lua_remove(L, 2);

                            argsToPush = (*sch).numLuaArgs;
                        }
                    }
                }

                if (lua_pcall(L, argsToPush, 0, 0) != 0)
                {
                    error("Error in schedule " + std::to_string((*sch).scheduleID) + " func: " + (*sch).functionName);
                    if (lua_gettop(L) > 0)
                    {
                        const char* err = lua_tostring(L, -1);
                        if (!err)
                            continue;
                        std::string errorstr = err;

                        int args = lua_gettop(L);
                        if (args > 0)
                            lua_pop(L, args);

                        replaceAll(errorstr, "[", "\\[");

                        error("[colour='FFFF0000']" + errorstr);
                    }
                }
            }

            sch = schedules.erase(sch);
        }
        else
            ++sch;
    }
    runningSchedules = false;

    for (unsigned int a = 0; a < schedulesThisFrame.size(); a++)
        schedules.push_back(schedulesThisFrame[a]);
    schedulesThisFrame.clear();

    for (unsigned int a = 0; a < tmpScheduleIDsToDelete.size(); a++)
    {
        for (unsigned int b = 0; b < schedules.size(); b++)
        {
            if (schedules[b].scheduleID == tmpScheduleIDsToDelete[a])
            {
                schedules.erase(schedules.begin() + b);
                break;
            }
        }
    }
    tmpScheduleIDsToDelete.clear();
}

int LuaScheduler::scheduleFunction(lua_State* L)
{
    scope("scheduleFunction");

    //A bare minimum of two arguments are required, the function name and the delay in milliseconds

    int args = lua_gettop(L);

    if (args < 2)
    {
        error("At least 2 arguments required!");
        lua_pushnil(L);
        return 1;
    }

    const char* functionName = lua_tostring(L, 2);

    if (!functionName)
    {
        error("Invalid function name string!");
        lua_pushnil(L);
        return 1;
    }

    int milliseconds = lua_tointeger(L, 1);
    if (milliseconds < 1)
    {
        error("Milliseconds must be > 0");
        lua_pushnil(L);
        return 1;
    }

    lua_remove(L, 2);
    lua_remove(L, 1);

    //Required 2 arguments handled, rest are optional parameters to the function call

    if (args > 2)
    {
        /*
            So the way schedule(time,name,...) variadic arguments are handled
            is that there's a global table in Lua called scheduleArgs
            which is a table of tables, each table being the arguments
        */

        lua_newtable(L);
        lua_insert(L, 1);

        for (int a = 2; a < args; a++)
        {
            lua_pushinteger(L, a - 1);
            lua_insert(L, -2);
            lua_settable(L, 1);
        }

        lua_getglobal(L, "scheduleArgs");
        if (!lua_istable(L, -1))
        {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_setglobal(L, "scheduleArgs");
            lua_getglobal(L, "scheduleArgs");
        }

        lua_insert(L, 1);

        lua_pushinteger(L, lastScheduleID);
        lua_insert(L, -2);

        lua_settable(L, 1);
        lua_pop(L, 1);
    }

    Schedule tmp;
    tmp.functionName = std::string(functionName);
    tmp.timeToExecute = SDL_GetTicks() + milliseconds;
    tmp.scheduleID = lastScheduleID;
    tmp.numLuaArgs = args - 2;

    //If we added a schedule to the loop of schedules while we were iterating over the loop running scheduled functions, it'd be an infinite loop
    if (runningSchedules)
        schedulesThisFrame.push_back(tmp);
    else
        schedules.push_back(tmp);

    //Lua function returns one argument, the ID of the schedule which can be used to cancel it
    lua_pushnumber(L, lastScheduleID);

    lastScheduleID++;

    return 1;
}

int LuaScheduler::cancelFunction(lua_State* L)
{
    scope("cancelFunction");

    unsigned int id = lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "scheduleArgs");
    if (!lua_istable(L, -1))
        error("Where did your scheduleArgs global table go!");
    else
    {
        //Erase the sub-table with this schedules arguments (if it exists) from scheduleArgs
        lua_pushinteger(L, id);
        lua_gettable(L, -2);

        if (lua_isnil(L, -1))
            lua_pop(L, 1);
        else
        {
            lua_pop(L, 1);

            lua_pushinteger(L, id);
            lua_pushnil(L);
            lua_settable(L, -3);
        }
    }
    lua_pop(L, 1);

    if (runningSchedules)
    {
        tmpScheduleIDsToDelete.push_back(id);
    }
    else
    {
        for (unsigned int a = 0; a < schedules.size(); a++)
        {
            if (schedules[a].scheduleID == id)
            {
                schedules.erase(schedules.begin() + a);
                return 0;
            }
        }
    }

    return 0;
}
