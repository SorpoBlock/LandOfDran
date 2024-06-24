#pragma once

#include "../NetTypes/NetType.h"
#include "Server.h"

//I wanted this to be a .tpp file since it's just templated code, but I guess MSVC doesn't like those

/*
	ObjHolder is a factory for SimObject instances
	It makes sure vector membership is synonymous with existance
	It can batch creation/update/removal packets on the server for more efficient networking
	It can manage Lua state
*/
template <typename T>
class ObjHolder
{
	/*
		Each time a SimObject is created, this will be assigned to it and increment by one
	*/
	static netIDType lastNetID;

	/*
		What type of objects does this hold
		There should be one ObjHolder per type of object
	*/
	SimObjectType type = SimObjectType::InvalidSimTypeId;

	/*
		This should be every instance of this type of object that currently exists in the program
	*/
	std::vector<std::shared_ptr<T>> allObjects;

	/*
		When an object gets deleted, its ID is added here
		Every frame or few frames one big packet is sent out to clients with all recent deletions
	*/
	std::vector<netIDType> recentlyDeletedIDs;

	/*
		When an object gets created, its pointer is added here
		Every frame or few frames one big packet is sent out to clients with all recent creations
	*/
	std::vector<std::shared_ptr<T>> recentCreations;

	//Will be nullptr if this is the client
	std::shared_ptr<Server> server = nullptr;

	public:

	/*
		Templated factory method for creating an object
		Sets ID and creation time, queues networking updates
	*/
	template <typename... Args>
	std::shared_ptr<T> create(Args... args)
	{
		std::shared_ptr<T> tmp(new T(args...));
		allObjects.push_back(std::move(tmp));
		allObjects.back()->netID = lastNetID++;
		allObjects.back()->creationTime = SDL_GetTicks();
		allObjects.back()->me = allObjects.back();
		allObjects.back()->onCreation();
		recentCreations.push_back(allObjects.back());
		return allObjects.back();
	}

	//Returns nullptr if object not found by that ID
	const inline std::shared_ptr<T> find(const netIDType& netID) const
	{
		auto hasNetID = [&netID](const std::shared_ptr<T>& query) { return query->netID == netID; };
		auto it = std::find_if(allObjects.begin(), allObjects.end(), hasNetID);
		return it != allObjects.end() ? *it : nullptr;
	}

	//How many objects of this type exist in the entire program
	const inline size_t size() const { return allObjects.size(); }

	//Used to delete an object you found as the result of ex. a collision callback where you just have the raw pointer
	inline void destroyByPointer(T* target)
	{
		auto hasPointer = [&target](const std::shared_ptr<T>& query) { return query.get() == target; };
		auto it = std::find_if(allObjects.begin(), allObjects.end(), hasPointer);
		if (it != allObjects.end())
		{
			recentlyDeletedIDs.push_back((*it)->netID);
			allObjects.erase(it);
		}
		else
		{
			scope("objHolder::destroy");
			error("Pointer not found in objHolder");
		}
	}

	//Exchange netID (i.e. from incoming client packet, lua callback) for vector index
	//Honestly no clue when this would be needed
	const inline size_t findIdx(const netIDType& netID) const
	{
		auto hasNetID = [&netID](const std::shared_ptr<T>& query) { return query->netID == netID; };
		auto it = std::find_if(allObjects.begin(), allObjects.end(), hasNetID);
		return it - allObjects.begin();
	}

	/*
		No guarentee the memory is actually deallocated here
		In particular, Lua variables may still hold onto a copy of the shared_ptr
		This would be the fault of bad Lua, and its safest to just let it retain that memory to avoid a crash
		In that case, the object will have a flag deleted = true that will cause any Lua calls on it to throw an error
	*/
	inline void destroy(std::shared_ptr<T>& idx)
	{
		auto it = std::find(allObjects.begin(), allObjects.end(), idx);
		if (it != allObjects.end())
		{
			(*it)->deleted = true;
			recentlyDeletedIDs.push_back((*it)->netID);
			allObjects.erase(it);
			idx.reset();
		}
		else
		{
			scope("objHolder::destroy");
			error("Pointer not found in objHolder");
		}
	}

	//Principle way of getting objects, outside of lua scripts and client packets that use netIDs
	inline const std::shared_ptr<T>& operator[](const std::size_t& idx) const
	{
		//This will mostly be used in loops so I'm not too worried about out of bounds indexing 
		/*scope("objHolder[]");
		if (idx >= allObjects.size())
		{
			error("SimObject index out of range.");
			return noObj;
		}
		else*/
			return allObjects[idx];
	}

	//Pass nullptr as server if this is being called from the client
	ObjHolder(SimObjectType _type,Server *_server = nullptr) : type(_type), server(_server)
	{
		ObjHolder<T>::lastNetID = 0;

		if (type == InvalidSimTypeId)
			error("ObjHolder::ObjHolder object type invalid");
	}

	~ObjHolder()
	{

	}
};