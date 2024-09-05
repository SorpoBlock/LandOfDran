#pragma once

#include "../LandOfDran.h"

#include "../Graphics/Mesh.h"
#include "../Physics/PhysicsWorld.h"
#include "../Networking/Quantization.h"
#include "NetType.h"
#include "../GameLoop/ClientProgramData.h"

/*
	Objects that can move around the scene, includes projectiles
	Holds at least a reference to a 3d Model that has visual mesh(es) and a collision box
	ItemType and PlayerType are derived from DynamicType
*/
class DynamicType : public NetType
{
	//DynamicType should create and own its Model
	std::shared_ptr<Model> model = nullptr; 

	//Added to collisionShape as a child shape
	btBoxShape* collisionBox = nullptr;

	//Adding a single box shape to a compound shape is how we can add an offset to it
	btCompoundShape* collisionShape = nullptr;

	//compound shapes require their own inertia calculations
	btVector3 defaultInertia = btVector3(0, 1, 0);

	//Set it to what you want 
	float mass = 1.0;

	btMotionState* defaultMotionState = nullptr;

	public:

	std::string scriptName = "";

	const glm::vec3& getScale() const { return model->baseScale; }

	std::shared_ptr<Model>  getModel() const { return model; }

	//Calls underlying model->render
	void render(std::shared_ptr<ShaderManager> graphics, bool useMaterials = true) const;

	//Called from lua scripts generally, file is relative path to text file containing model loading settings 
	//Increment ID after using
	void serverSideLoad(const std::string &filePath, netIDType typeID, glm::vec3 baseScale);

	//Net types as opposed to SimObjects always just get their own packet
	//This is the packet from createTypePacket
	virtual void loadFromPacket(ENetPacket const* const packet, const ClientProgramData &pd) override;

	//Create the packet on the server to be passed to loadFromPacket on the client
	virtual ENetPacket* createTypePacket() const override;

	//Creates a physics object using this type's collision shape
	btRigidBody* createBody() const;
	btRigidBody* createBodyStatic(const btTransform &t) const;

	//Doesn't allocate anything, wait for loadFromPacket or server-side construction from lua
	DynamicType();

	//Be sure not to call this until all objects of this type have been destroyed, deallocates models and physics data
	~DynamicType();
};



