#include "LoopServer.h"

void LoopServer::run(float deltaT, ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	server->run(&pd); //   <---- networking
	pd.physicsWorld->step(deltaT);
}

LoopServer::LoopServer(ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	server = new Server(DEFAULT_PORT);
	if (!server->isValid())
		return;

	///Server just has one physics world that's started when the program starts and stays until shutdown, unlike client
	pd.physicsWorld = std::make_shared<PhysicsWorld>();
	SimObject::world = pd.physicsWorld;

	pd.dynamics = new ObjHolder<Dynamic>(SimObjectType::DynamicTypeId, server);

	//Test:
	auto testType = std::make_shared<DynamicType>();
	testType->serverSideLoad("Assets/brickhead/brickhead.txt",pd.dynamicTypes.size());
	pd.dynamicTypes.push_back(testType);
	pd.allNetTypes.push_back(testType);
	auto testObject = pd.dynamics->create(testType, btVector3(0, 0, 0));

	valid = true;
}

LoopServer::~LoopServer()
{
	pd.physicsWorld.reset();
	SimObject::world = nullptr;

	delete pd.dynamics;

	pd.dynamicTypes.clear();
	pd.allNetTypes.clear();

	delete server;
}
