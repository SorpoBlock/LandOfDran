#include "LoopServer.h"

#include "../LuaFunctions/Dynamic.h"

void LoopServer::run(float deltaT, ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	server->run(&pd); //   <---- networking
	pd.dynamics->sendRecent();
	pd.physicsWorld->step(deltaT);

	for (unsigned int a = 0; a < Logger::getStorage()->size(); a++)
		server->updateAdminConsoles(Logger::getStorage()->at(a));
	Logger::getStorage()->clear();

	scheduler->run(pd.luaState);
}

LoopServer::LoopServer(ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	server = new Server(DEFAULT_PORT);
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

	///Server just has one physics world that's started when the program starts and stays until shutdown, unlike client
	pd.physicsWorld = std::make_shared<PhysicsWorld>();
	SimObject::world = pd.physicsWorld;

	pd.dynamics = new ObjHolder<Dynamic>(SimObjectType::DynamicTypeId, server);
	pd.dynamics->makeLuaMetatable(pd.luaState, "metatable_dynamic", getDynamicFunctions(pd.luaState));

	//Test:
	auto testType = std::make_shared<DynamicType>();
	testType->serverSideLoad("Assets/brickhead/brickhead.txt",pd.dynamicTypes.size());
	pd.dynamicTypes.push_back(testType);
	pd.allNetTypes.push_back(testType);
	//End test

	valid = true;
}

LoopServer::~LoopServer()
{
	delete scheduler;

	LUA_args = nullptr;
	LUA_pd = nullptr;

	pd.physicsWorld.reset();
	SimObject::world = nullptr;

	delete pd.dynamics;

	pd.dynamicTypes.clear();
	pd.allNetTypes.clear();

	delete server;
}
