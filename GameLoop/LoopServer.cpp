#include "LoopServer.h"

#include "../LuaFunctions/Dynamic.h"
#include "../LuaFunctions/Static.h"

template <>
netIDType ObjHolder<Dynamic>::lastNetID = 0;
netIDType ObjHolder<StaticObject>::lastNetID = 0;

void LoopServer::run(float deltaT, ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	server->run(&pd,pd.luaState,pd.eventManager); //   <---- networking
	pd.dynamics->sendRecent();
	pd.statics->sendRecent();
	pd.physicsWorld->step(deltaT); 

	for (unsigned int a = 0; a < Logger::getStorage()->size(); a++)
		server->updateAdminConsoles(Logger::getStorage()->at(a));
	Logger::getStorage()->clear();

	for (unsigned int a = 0; a < pd.clients.size(); a++)
	{
		for (unsigned int b = 0; b < pd.clients[a]->controllers.size(); b++)
		{
			pd.clients[a]->controllers[b].controlWithLastInput(pd.physicsWorld,deltaT);
		}
	}

	scheduler->run(pd.luaState);
}

LoopServer::LoopServer(ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	server = new Server(DEFAULT_PORT);
	LUA_server = server;
	if (!server->isValid())
		return;

	LUA_args = &cmdArgs;
	LUA_pd = &pd;

	pd.evalPassword = settings->getString("hosting/evalpassword");
	pd.useEvalPassword = settings->getBool("hosting/useevalpassword");
	if (pd.evalPassword == " " || pd.evalPassword == "changeme" || pd.evalPassword.length() < 1)
		pd.useEvalPassword = false;
	//TODO: Hash password

	//Start up Lua and give it access to all the default libraries, file io, debugging, math, etc.
	pd.luaState = luaL_newstate();
	luaL_openlibs(pd.luaState);
	registerOtherFunctions(pd.luaState);
	scheduler = new LuaScheduler(pd.luaState);
	pd.eventManager = new EventManager(pd.luaState);
	registerClientFunctions(pd.luaState);

	///Server just has one physics world that's started when the program starts and stays until shutdown, unlike client
	pd.physicsWorld = std::make_shared<PhysicsWorld>();
	SimObject::world = pd.physicsWorld;

	pd.dynamics = new ObjHolder<Dynamic>(SimObjectType::DynamicTypeId, server);
	pd.dynamics->makeLuaMetatable(pd.luaState, "metatable_dynamic", getDynamicFunctions(pd.luaState));
	pd.statics = new ObjHolder<StaticObject>(SimObjectType::StaticTypeId, server);
	pd.statics->makeLuaMetatable(pd.luaState, "metatable_static", getStaticFunctions(pd.luaState));

	info("Loading serverstart.lua");

	if (luaL_dofile(pd.luaState, "serverstart.lua"))
	{
		error("Error loading serverstart.lua: " + std::string(lua_tostring(pd.luaState, -1)));
		info("Input any text to exit.");
		std::string holdForAWhile;
		std::cin >> holdForAWhile;
		valid = false;
		return;
	}

	info("Server running");

	if(pd.allNetTypes.size() == 0)
		error("No NetTypes registered, clients will freeze on joining, no objects can be created.");

	valid = true;
}

LoopServer::~LoopServer()
{
	delete pd.eventManager;
	delete scheduler;

	LUA_args = nullptr;
	LUA_pd = nullptr;

	pd.physicsWorld.reset();
	SimObject::world = nullptr;

	delete pd.dynamics;
	delete pd.statics;

	pd.dynamicTypes.clear();
	pd.allNetTypes.clear();

	delete server;
	LUA_server = nullptr;
}
