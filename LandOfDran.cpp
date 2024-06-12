// LandOfDran.cpp : Defines the entry point for the application.

#include "LandOfDran.h"
#include "Utility/SettingManager.h"
#include "Utility/DefaultPreferences.h"
#include "Utility/GlobalStartup.h"
#include "Utility/ClientData.h"
#include "Graphics/ShaderSpecification.h"
#include "Graphics/Material.h"
#include "Graphics/Mesh.h"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>
 
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

	textures.allocateForDecals(128);
	textures.addDecal("Assets/animan.png", 0);
	textures.addDecal("Assets/ascii-terror.png", 1);
	textures.finalizeDecals();

	Model test("Assets/Gun/gun.txt",&textures); 
	for (unsigned int a = 0; a < 10; a++)
	{
		for (unsigned int b = 0; b < 10; b++)
		{
			for (unsigned int c = 0; c < 10; c++)
			{
				ModelInstance* tester = new ModelInstance(&test);
				tester->setModelTransform(glm::translate(glm::vec3(a*5,b*5,c*5)));
				tester->update();
			}
		}
	}

	grass.use(&shaders);
	shaders.modelShader->registerUniformFloat("test", true, 0.5);

	shaders.cameraUniforms.CameraView = glm::lookAt(glm::vec3(0, -1, -3), glm::vec3(0,0,1), glm::vec3(0,1,0));
	shaders.cameraUniforms.CameraProjection = glm::perspective(glm::radians(90.0), 1.0, 0.1, 400.0);
	shaders.updateCameraUBO();

	GLuint vao = createQuadVAO();

	float angle = 0.0;
	float lastTicks = SDL_GetTicks();
	unsigned int frames = 0;
	unsigned int lastFPScheck = SDL_GetTicks();

	bool whichToUse = false;
	bool asdf = false;

	bool doMainLoop = true;
	while (doMainLoop)
	{
		frames++;
		if (SDL_GetTicks() > lastFPScheck + 5000)
		{
			info(std::to_string(frames / 5) + " fps");
			lastFPScheck = SDL_GetTicks();
			frames = 0;
			whichToUse = !whichToUse;
			if (!whichToUse)
				asdf = !asdf;
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

		shaders.basicUniforms.RotationMatrix = glm::eulerAngleXYZ(-sin(angle), 0.f, 0.f);
		shaders.basicUniforms.useDecal = asdf ? 0 : 1;
		shaders.updateBasicUBO();

		context.select(); 
		context.clear(1,1,1);

		shaders.modelShader->use();
		test.render(&shaders);

		/*glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);*/
		
		context.swap();
	}

	globalShutdown();
	
	return 0;
}
