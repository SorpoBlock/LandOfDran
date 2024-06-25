#pragma once

#include "../Networking/ObjHolder.h"
#include "../Physics/PhysicsWorld.h"

/*
	Abstract base class for anything which has state that is maintained by the server
	and is referenced on the client.
*/
class SimObject
{
	//Should be the factory class that creates and deletes all SimObjects
	friend ObjHolder<SimObject>;

	protected:

	explicit SimObject();

	//Set by ObjHolder in milliseconds since program start:
	uint32_t creationTime = 0;
	
	/*
		Set by ObjHolder on creation
		Unique to objects of this type, i.e.no 2 dynamics will ever share an id, but a brick and a dynamic might
	*/
	netIDType netID = 0;

	/*
		Has something changed on this object such that clients need to be sent an update over the internet
		since the last time updates were sent out on this object type
	*/
	bool requiresUpdate = false;
	
	/*
		Set when we formally delete it using ObjHolder, though theroetically bad lua code may still hold this object in memory via shared_ptrs
		If this is true any Lua call on it will cause an error
	*/
	bool deleted = false;

	/*
		Set by ObjHolder on creation
		Allows the object to give out a common smart pointer to itself instead of 'this' to other objects it makes that may want a reference back to it
	*/
	std::shared_ptr<SimObject> me;

	public:

	//Never use 'this' always use this
	std::shared_ptr<SimObject> getMe() const
	{
		return me;
	}

	//Yes, global scope is bad and all but we really can only have one dynamicsWorld and it's needed constantly by everything
	//The methods I used to use to get around making this global were honestly far worse
	static std::shared_ptr<PhysicsWorld> world;

	//Has this object been updated in such a way that we need to resend its properties to clients?
	virtual bool requiresNetUpdate() const;

	//Get netID
	netIDType getID() const;

	//Time in MS since program start
	uint32_t getCreationTime() const;

	//Called after this object has an id, type, me pointer, and vector index assigned by ObjHolder
	virtual void onCreation() = 0;

	//How many bytes would this add to a packet creating objects if it was added to it
	virtual unsigned int getCreationPacketBytes() const = 0;

	//How many bytes would this add to a packet updating objects if it was added to it
	virtual unsigned int getUpdatePacketBytes() const = 0;

	//Add getCreationPacketBytes() worth of data to the given packet with all the data needed for the client to create it
	virtual void addToCreationPacket(enet_uint8* dest) const = 0;

	//Add getUpdatePacketBytes() worth of data to the given packet with all the data needed for the client to update it
	virtual void addToUpdatePacket(enet_uint8* dest) = 0;

	virtual ~SimObject();
};

