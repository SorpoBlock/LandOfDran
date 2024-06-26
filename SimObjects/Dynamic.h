#pragma once

#include "../Networking/ObjHolder.h"
#include "../SimObjects/SimObject.h"
#include "../NetTypes/DynamicType.h"
#include "../Graphics/Interpolator.h"
#include "../Networking/Quantization.h"
#include "../Utility/GlobalStartup.h" //getTicksMS

/*
	Dynamics are basically any object that can move around with physics
	Projectiles are dynamics and items and players are child classes of dynamics
*/
class Dynamic : public SimObject
{
	friend ObjHolder<Dynamic>;
	 
	protected:

	/*
		The last transform we sent with addToUpdatePacket
	*/
	btTransform lastSentTransform = btTransform::getIdentity();
	
	//Time with SDL_GetTicks that we sent lastSentTransform
	unsigned int lastSentTime = 0;

	/*
		Determines its physical appearance and physics properties
	*/
	std::shared_ptr<DynamicType> type = nullptr;

	//Other constructor is always client side, this is probably always going to be server-side but I'm not sure yet
	explicit Dynamic(std::shared_ptr<DynamicType> _type, const btVector3& initialPos);

	//Called after this object has an id, type, me pointer, and vector index assigned by ObjHolder
	virtual void onCreation() override;

	//Client only: holds buffer data for instanced mesh rendering
	ModelInstance* modelInstance = nullptr;

	//Physics object
	btRigidBody* body = nullptr;

	//Called by objHolder when destroy is first called, gives object an oppertunity to reset smart pointers it might have
	virtual void requestDestruction() override;

	public:

	void updateSnapshot();

	//Client only
	Interpolator interpolator;

	//Server only, used to set physics body position
	void setPosition(const btVector3& pos);

	//Server only, used to set physics body linear veclotiy
	void setVelocity(const btVector3& vel);

	btVector3 getVelocity();

	virtual bool requiresNetUpdate() const override;

	//How many bytes would this add to a packet creating objects if it was added to it
	virtual unsigned int getCreationPacketBytes() const override;

	//How many bytes would this add to a packet updating objects if it was added to it
	virtual unsigned int getUpdatePacketBytes() const override;

	//Add getCreationPacketBytes() worth of data to the given packet with all the data needed for the client to create it
	virtual void addToCreationPacket(enet_uint8 * dest) const override;

	//Add getUpdatePacketBytes() worth of data to the given packet with all the data needed for the client to update it
	virtual void addToUpdatePacket(enet_uint8 * dest) override;

	~Dynamic();
};

//Helper function to get a normal dynamic shared_ptr from its btRigidBody with a ton of error checking, returns nullptr on error
std::shared_ptr<Dynamic> dynamicFromBody(const btRigidBody* in);

