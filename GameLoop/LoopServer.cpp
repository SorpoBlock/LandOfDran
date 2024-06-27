#include "LoopServer.h"

void LoopServer::run(float deltaT, ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	server->run(&pd); //   <---- networking
	pd.dynamics->sendRecent();
	pd.physicsWorld->step(deltaT);

	//Testing only:
	spin += deltaT * 0.002f;
	if (pd.dynamics->get(0))
		pd.dynamics->get(0)->setVelocity(btVector3(sin(spin)*10.0,0,cos(spin)*10.0));
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
	//End test

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
