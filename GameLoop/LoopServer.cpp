#include "LoopServer.h"

void LoopServer::run(float deltaT, ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	server->run();
}

LoopServer::LoopServer(ExecutableArguments& cmdArgs, std::shared_ptr<SettingManager> settings)
{
	server = new Server(DEFAULT_PORT);
	if (!server->isValid())
		return;

	valid = true;
}

LoopServer::~LoopServer()
{
	delete server;
}
