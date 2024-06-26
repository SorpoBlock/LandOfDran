#pragma once

#include "../NetTypes/NetType.h"
#include "Server.h"

//Basically we wanna make sure packets are under the MTU if possible,
//but I'm not positive how much overhead ENet adds to packets
#define MTU ENET_HOST_DEFAULT_MTU - 20

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

	//Force the ID of the next created object to be a certain ID
	//This is used because the client gets object ids from the server
	inline void clientSetNextId(const netIDType& netID)
	{
		if (server)
		{
			error("clientSetNextId should only be called from the client!");
			return;
		}

		lastNetID = netID;
	}

	//Returns nullptr if object not found by that ID
	const inline std::shared_ptr<T> find(const netIDType& netID) const
	{
		if (allObjects.size() == 0)
			return nullptr;

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
	inline std::shared_ptr<T> operator[](const std::size_t& idx) const
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

	//Array operator sometimes doesn't compile
	inline std::shared_ptr<T> get(const std::size_t& idx) const
	{
		return allObjects[idx];
	}

	//Call on client when they confirm phase 1 loading is complete
	void sendAll(JoinedClient const *client) const
	{
		int toSend = allObjects.size();
		int sent = 0;

		//Send as many packets as needed to send all objects to the client
		//without any one packet exceeding the MTU
		while (toSend > 0)
		{
			int sentThisPacket = 0;
			int bytesThisPacket = 0;

			//I guess if one object was somehow larger than like 1300 bytes this would stall lol
			for (unsigned int a = sent; a < allObjects.size(); a++)
			{
				//Only using one byte to encode amount of objects in the packet
				if (sentThisPacket >= 255)
					break;

				sentThisPacket++;
				bytesThisPacket += allObjects[a]->getCreationPacketBytes();

				if (bytesThisPacket > MTU)
				{
					sentThisPacket--;
					bytesThisPacket -= allObjects[a]->getCreationPacketBytes();
					break;
				}
			}

			//Three extra bytes, packet type, simobject type, amount of objects
			ENetPacket* packet = enet_packet_create(NULL, bytesThisPacket + 3, getFlagsFromChannel(OtherReliable));
			packet->data[0] = FromServerPacketType::AddSimObjects;
			packet->data[1] = SimObjectType::DynamicTypeId;
			packet->data[2] = sentThisPacket;
			int byteIterator = 3;

			for (int a = sent; a < sent + sentThisPacket; a++)
			{
				allObjects[a]->addToCreationPacket(packet->data + byteIterator);
				byteIterator += allObjects[a]->getCreationPacketBytes();
			}

			client->send(packet, OtherReliable);
			toSend -= sentThisPacket;
			sent += sentThisPacket;
		}
	}

	//Sends all recent creations, deletions, and updates to objects it manages to all connected clients
	void sendRecent()
	{
		if (server->getNumClients() == 0)
		{
			recentCreations.clear();
			recentlyDeletedIDs.clear();
			return;
		}

		//Send recently created objects to all connected clients:

		int toSend = recentCreations.size();
		int sent = 0;

		//Send as many packets as needed to send all objects to the client
		//without any one packet exceeding the MTU
		while (toSend > 0)
		{
			int sentThisPacket = 0;
			int bytesThisPacket = 0;

			//I guess if one object was somehow larger than like 1300 bytes this would stall lol
			for (unsigned int a = sent; a < recentCreations.size(); a++)
			{
				//Only using one byte to encode amount of objects in the packet
				if (sentThisPacket >= 255)
					break;

				sentThisPacket++;
				bytesThisPacket += recentCreations[a]->getCreationPacketBytes();

				if (bytesThisPacket > MTU)
				{
					sentThisPacket--;
					bytesThisPacket -= recentCreations[a]->getCreationPacketBytes();
					break;
				}
			}

			if (sentThisPacket == 0)
				break;

			//Three extra bytes, packet type, simobject type, amount of objects
			ENetPacket* packet = enet_packet_create(NULL, bytesThisPacket + 3, getFlagsFromChannel(OtherReliable));
			packet->data[0] = FromServerPacketType::AddSimObjects;
			packet->data[1] = SimObjectType::DynamicTypeId;
			packet->data[2] = sentThisPacket;
			int byteIterator = 3;

			for (int a = sent; a < sent + sentThisPacket; a++)
			{
				recentCreations[a]->addToCreationPacket(packet->data + byteIterator);
				byteIterator += recentCreations[a]->getCreationPacketBytes();
			}

			server->broadcast(packet, OtherReliable);
			toSend -= sentThisPacket;
			sent += sentThisPacket;
		}

		recentCreations.clear();

		//Send recently deleted objects(' IDs) to all connected clients:

		toSend = recentlyDeletedIDs.size();
		sent = 0;

		//Send as many packets as needed to send all objects to the client
		//without any one packet exceeding the MTU
		while (toSend > 0)
		{
			int sentThisPacket = 0;
			int bytesThisPacket = 0;

			for (unsigned int a = sent; a < recentCreations.size(); a++)
			{
				//Only using one byte to encode amount of objects in the packet
				if (sentThisPacket >= 255)
					break;

				sentThisPacket++;
				bytesThisPacket += sizeof(netIDType);

				if (bytesThisPacket > MTU)
				{
					sentThisPacket--;
					bytesThisPacket -= sizeof(netIDType);
					break;
				}
			}

			if (sentThisPacket == 0)
				break;

			//Three extra bytes, packet type, simobject type, amount of objects
			ENetPacket* packet = enet_packet_create(NULL, bytesThisPacket + 3, getFlagsFromChannel(OtherReliable));
			packet->data[0] = FromServerPacketType::DeleteSimObjects;
			packet->data[1] = SimObjectType::DynamicTypeId;
			packet->data[2] = sentThisPacket;
			int byteIterator = 3;

			for (int a = sent; a < sent + sentThisPacket; a++)
			{
				memcpy(packet->data + byteIterator, &recentlyDeletedIDs[a], sizeof(netIDType));
				byteIterator += sizeof(netIDType);
			}

			server->broadcast(packet, OtherReliable);
			toSend -= sentThisPacket;
			sent += sentThisPacket;
		}

		recentlyDeletedIDs.clear();

		//Send any pending object updates to all connected clients:

		toSend = allObjects.size();
		sent = 0;

		//Send as many packets as needed to send all objects to the client
		//without any one packet exceeding the MTU
		while (toSend > 0)
		{
			//Didn't need updates, didn't move or get changed since last time
			int skippedThisPacket = 0;
			//Did need updates
			int sentThisPacket = 0;
			int bytesThisPacket = 0;

			//I guess if one object was somehow larger than like 1300 bytes this would stall lol
			for (unsigned int a = sent; a < allObjects.size(); a++)
			{
				//Only using one byte to encode amount of objects in the packet
				if (sentThisPacket >= 255)
					break;

				if (!allObjects[a]->requiresNetUpdate())
				{
					skippedThisPacket++;
					continue;
				}

				sentThisPacket++;
				bytesThisPacket += allObjects[a]->getUpdatePacketBytes();

				if (bytesThisPacket > MTU)
				{
					sentThisPacket--;
					bytesThisPacket -= allObjects[a]->getUpdatePacketBytes();
					break;
				}
			}

			//Not a break: we may have just skipped every object in this packet
			if (sentThisPacket == 0)
				continue;

			//Three extra bytes, packet type, simobject type, amount of objects
			ENetPacket* packet = enet_packet_create(NULL, bytesThisPacket + 3, getFlagsFromChannel(OtherReliable));
			packet->data[0] = FromServerPacketType::UpdateSimObjects;
			packet->data[1] = SimObjectType::DynamicTypeId;
			packet->data[2] = sentThisPacket;
			int byteIterator = 3;

			for (int a = sent; a < sent + sentThisPacket; a++)
			{
				if (!allObjects[a]->requiresNetUpdate())
					continue;
				allObjects[a]->addToUpdatePacket(packet->data + byteIterator);
				byteIterator += allObjects[a]->getUpdatePacketBytes();
			}

			server->broadcast(packet, OtherReliable);
			toSend -= (sentThisPacket + skippedThisPacket);
			sent += (sentThisPacket + skippedThisPacket);
		}
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