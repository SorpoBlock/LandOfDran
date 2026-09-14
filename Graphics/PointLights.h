#pragma once

#include "../LandOfDran.h"
#include "ShaderSpecification.h"
#include "RenderTarget.h"

//What PointLights needs to know about one Light this frame, copied out so this header doesn't pull in the SimObject classes
struct PointLightSource
{
	netIDType id = 0;
	//Already moved by flicker
	glm::vec3 position = glm::vec3(0);
	glm::vec3 color = glm::vec3(1);
	float brightness = 0;
	float coronaWidth = 0;
	float range = 0;
	//Already turned by spin
	glm::vec3 direction = glm::vec3(0, -1, 0);
	//Cosine of half a spotlight's cone angle, below -1 for a light that shines every way
	float spotCosine = -2;
};

/*
	Client only: picks which Lights to draw each frame, renders shadow maps for the ones nearest the camera, and draws coronae
	The chosen lights reach shaders through PointLightUniforms, see ShaderSpecification.h and model.frag
	Each shadowed light gets six layers of one depth texture array, one per cube face, which only needs OpenGL 3.3
*/
class PointLights
{
	struct ShadowSlot
	{
		netIDType lightID = NO_ID;

		//The light as of this frame and its view of each cube face, set by update
		glm::vec3 position = glm::vec3(0);
		float range = 0;
		glm::vec3 direction = glm::vec3(0);
		float spotCosine = -2;
		glm::mat4 faces[6];
		//A spotlight's beam can't reach every face, so those aren't drawn
		bool faceNeeded[6] = { true, true, true, true, true, true };

		//As of the last time its faces were drawn
		bool rendered = false;
		glm::vec3 renderedPosition = glm::vec3(0);
		float renderedRange = 0;
		glm::vec3 renderedDirection = glm::vec3(0);
		float renderedSpotCosine = -2;
		unsigned int renderedSceneGeneration = 0;
		bool movingCastersNear = false;
	};

	ShadowSlot slots[PointLightUniforms::maxShadowed];
	int shadowCount = 0;
	int faceResolution = 0;
	std::shared_ptr<RenderTarget> shadowMaps = nullptr;

	//Per corona: position, width, then color and strength
	std::vector<float> coronaInstances;
	GLuint coronaVao = 0;
	GLuint coronaBuffer = 0;
	GLsizei coronaCount = 0;

	//For the debug menu
	int litCount = 0;
	int shadowedCount = 0;
	int facesDrawn = 0;

	public:

	//graphics/pointshadows and the size of each cube face, recreates the shadow maps if either changed
	void setShadowSettings(int count, int resolution, std::shared_ptr<TextureManager> textures);

	/*
		Picks the lights that can light or be seen from the view: the nearest maxLit are lit, and the nearest of those get the shadow slots
		Uploads PointLightUniforms and this frame's coronae
	*/
	void update(const std::vector<PointLightSource>& lights, std::shared_ptr<ShaderManager> shaders, const glm::vec3& cameraPosition, const glm::mat4& viewProjection, float fogEnd);

	/*
		Redraws the cube faces of shadowed lights whose shadows could have changed: new to their slot, moved or turned, sceneGeneration changed since,
		or movingCastersNear says something that moves is within their range now or was the last time they were checked
		drawCasters is called with the light's view of one face, while that face's layer is bound and cleared
	*/
	void renderShadows(unsigned int sceneGeneration, const std::function<bool(const glm::vec3& position, float range)>& movingCastersNear, const std::function<void(const glm::mat4& lightSpaceMatrix)>& drawCasters);

	void bindShadowMaps() const;

	//Additive glow billboards, drawn after everything that writes depth, sets its own blending
	void renderCoronae(std::shared_ptr<ShaderManager> shaders) const;

	std::string getStats() const;

	PointLights();
	~PointLights();
};
