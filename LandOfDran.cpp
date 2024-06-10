// LandOfDran.cpp : Defines the entry point for the application.

#include "LandOfDran.h"
#include "Utility/SettingManager.h"
#include "Utility/DefaultPreferences.h"
#include "Utility/GlobalStartup.h"
#include "Utility/ClientData.h"
#include "Graphics/ShaderSpecification.h"
#include "Graphics/Material.h"
#include <glm/gtx/transform.hpp>
 
using namespace std;

int main(int argc, char* argv[])
{
	/*
		Holds everything we need for playing the game
		Not needed if this is a dedicated server
		Everything inside should be allocated and populated otherwise
		Nothing should be removed or deallocated until program shutdown
	*/
	ClientData clientEnv;
	
	Logger::setErrorFile("Logs/error.txt");
	Logger::setInfoFile("Logs/log.txt");

	info("Starting Land of Dran");  

	info("Opening preferences file");

	//Import settings we have, set defaults for settings we don't, then export the file with any new default values
	SettingManager preferences("Config/settings.txt");
	populateDefaults(preferences);
	preferences.exportToFile("Config/settings.txt");
	clientEnv.preferences = &preferences;

	Logger::setDebug(preferences.getBool("logger/verbose"));

	//Start SDL and other libraries
	globalStartup(preferences);

	//Create our program window
	RenderContext context(preferences);
	clientEnv.renderContext = &context;

	TextureManager textures;

	Material grass("Assets/grass/grass.txt", &textures);
	 
	//Load all the shaders
	ShaderManager shaders;
	clientEnv.shaders = &shaders;
	shaders.readShaderList("Shaders/shadersList.txt");

	shaders.modelShader->registerUniformFloat("test", true, 0.5);

	shaders.globalUniforms.CameraView = glm::lookAt(glm::vec3(0, 0, -1), glm::vec3(0,0,1), glm::vec3(0,1,0));
	shaders.globalUniforms.CameraProjection = glm::perspective(glm::radians(90.0), 1.0, 0.1, 400.0);
	shaders.updateUniformBlock();

	GLuint vao = createQuadVAO();

	float angle = 0.0;
	float lastTicks = SDL_GetTicks();
	unsigned int frames = 0;
	unsigned int lastFPScheck = SDL_GetTicks();

	bool doMainLoop = true;
	while (doMainLoop)
	{
		frames++;
		if (SDL_GetTicks() > lastFPScheck + 5000)
		{
			info(std::to_string(frames / 5) + " fps");
			lastFPScheck = SDL_GetTicks();
			frames = 0;
		}

		float deltaT = ((float)SDL_GetTicks()) - lastTicks;
		lastTicks = SDL_GetTicks();
		angle += deltaT * 0.001;

		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
			{
				doMainLoop = false;
				break;
			}
		}

		shaders.globalUniforms.CameraView = glm::lookAt(glm::vec3(-sin(angle), 0, -cos(angle)), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		shaders.updateUniformBlock();

		context.select(); 
		context.clear(1,1,1);

		shaders.modelShader->use();
		grass.use(&shaders);

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
		context.swap();
	}

	globalShutdown();
	
	return 0;
}
