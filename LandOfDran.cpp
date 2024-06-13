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
	//Material grass("Assets/grass/grass.txt", &textures);
	 
	//Load all the shaders
	ShaderManager shaders;
	clientEnv.shaders = &shaders;
	shaders.readShaderList("Shaders/shadersList.txt");

	textures.allocateForDecals(128);
	textures.addDecal("Assets/animan.png", 0);
	textures.addDecal("Assets/ascii-terror.png", 1);
	textures.finalizeDecals();

	Model test("Assets/brickhead/brickhead.txt",&textures); 
	test.baseScale = glm::vec3(0.01);
	test.printHierarchy();

	std::vector<ModelInstance*> instances;
	for (unsigned int a = 0; a < 1; a++)
	{
		for (unsigned int b = 0; b < 1; b++)
		{
			for (unsigned int c = 0; c < 1; c++)
			{
				ModelInstance* tester = new ModelInstance(&test);
				instances.push_back(tester);
				tester->setModelTransform(glm::translate(glm::vec3(a * 8, b * 8, c * 8)));
				tester->update(false);
			} 
		}
	} 

	//grass.use(&shaders);
	shaders.modelShader->registerUniformFloat("test", true, 0.5);

	float yaw = 0;
	float pitch = 0;
	glm::vec3 camPos  = glm::vec3(0, -1, -3);
	glm::vec3 camDir  = glm::vec3(sin(yaw) * cos(pitch), sin(pitch), cos(yaw) * cos(pitch));
	glm::vec3 perpDir = glm::vec3(sin(yaw+1.57) * cos(pitch), sin(pitch), cos(yaw + 1.57) * cos(pitch));

	shaders.cameraUniforms.CameraProjection = glm::perspective(glm::radians(90.0), 1.0, 0.1, 400.0);
	shaders.cameraUniforms.CameraView = glm::lookAt(camPos, glm::vec3(0,0,1), glm::vec3(0,1,0));
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

		bool camUpdate = false;
		const Uint8* states = SDL_GetKeyboardState(NULL);

		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
			{
				doMainLoop = false;
				break;
			}
			else if (e.type == SDL_MOUSEMOTION && context.getMouseLocked())
			{
				camUpdate = true;
				yaw -= e.motion.xrel / 100.0;
				pitch -= e.motion.yrel / 100.0;
				if (pitch < -1.57)
					pitch = -1.57;
				if (pitch > 1.57)
					pitch = 1.57;
			}
			else if (e.type == SDL_KEYDOWN)
			{
				if (e.key.keysym.sym == SDLK_m)
					context.setMouseLock(!context.getMouseLocked());
			}
		}

		if (states[SDL_SCANCODE_W])
		{
			camPos += glm::vec3(deltaT * 0.01) * camDir;
			camUpdate = true;
		}
		else if (states[SDL_SCANCODE_S])
		{
			camPos -= glm::vec3(deltaT * 0.01) * camDir;
			camUpdate = true;
		}
		else if (states[SDL_SCANCODE_A])
		{
			camPos += glm::vec3(deltaT * 0.01) * perpDir;
			camUpdate = true;
		}
		else if (states[SDL_SCANCODE_D])
		{
			camPos -= glm::vec3(deltaT * 0.01) * perpDir;
			camUpdate = true;
		}

		if (camUpdate)
		{
			camDir = glm::vec3(sin(yaw) * cos(pitch), sin(pitch), cos(yaw) * cos(pitch));
			perpDir = glm::vec3(sin(yaw + 1.57) * cos(pitch), sin(pitch), cos(yaw + 1.57) * cos(pitch));
			shaders.cameraUniforms.CameraView = glm::lookAt(camPos, camPos+camDir, glm::vec3(0, 1, 0));
			shaders.cameraUniforms.CameraPosition = camPos;
			shaders.cameraUniforms.CameraDirection = camDir;
			shaders.updateCameraUBO();
		}

		/*for (unsigned int a = 0; a < 10; a++)
		{
			for (unsigned int b = 0; b < 10; b++)
			{
				for (unsigned int c = 0; c < 10; c++)
				{
					instances[c + b * 10 + a * 100]->setModelTransform(glm::translate(glm::vec3(a * 8, b * 8, c * 8)) * glm::toMat4(glm::quat(glm::vec3(1.57, 1.57, angle))));
					instances[c + b * 10 + a * 100]->calculateMeshTransforms();
					//instances[c + b * 10 + a * 100]->update();
				}
			}
		}
		test.recompileAll();*/

		context.select(); 
		context.clear(0.2,0.2,0.2);

		shaders.modelShader->use();
		test.render(&shaders);
		
		context.swap();
	}

	globalShutdown();
	
	return 0;
}
