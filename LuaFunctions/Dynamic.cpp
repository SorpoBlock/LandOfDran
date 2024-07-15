#include "Dynamic.h"

ServerProgramData* LUA_pd = nullptr;

static int LUA_createDynamic(lua_State* L)
{
	scope("(LUA) createDynamic");

	int args = lua_gettop(L);

	if(args != 4)
	{
		error("Expected 4 arguments createDynamic(typeID, x, y, z)");
		return 0;
	}

	int typeID = lua_tointeger(L, 1);
	float x = lua_tonumber(L, 2);
	float y = lua_tonumber(L, 3);
	float z = lua_tonumber(L, 4);

	lua_pop(L, 4);

	if(!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	if (typeID >= LUA_pd->dynamicTypes.size() || typeID < 0)
	{
		error("typeID out of range");
		return 0;
	}

	std::shared_ptr<DynamicType> type = LUA_pd->dynamicTypes[typeID];
	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->create(type, btVector3(x, y, z));

	LUA_pd->dynamics->pushLua(L, dynamic);
	return 1;
}

static int LUA_dynamicDestroy(lua_State* L)
{
	scope("(LUA) dynamic:destroy");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 argument dynamic:destroy()");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	LUA_pd->dynamics->destroy(dynamic);

	return 0;
}

static int LUA_dynamicSetVelocity(lua_State* L)
{
	scope("(LUA) dynamic:setVelocity");

	int args = lua_gettop(L);

	if (args != 4)
	{
		error("Expected 4 arguments dynamic:setVelocity(x,y,z)");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	float z = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float y = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float x = lua_tonumber(L, -1);
	lua_pop(L, 1);

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	dynamic->setVelocity(btVector3(x, y, z));
	dynamic->activate();

	return 0;
}

static int LUA_dynamicSetAngularFactor(lua_State* L)
{
	scope("(LUA) dynamic:setAngularFactor");

	int args = lua_gettop(L);

	if (args != 4)
	{
		error("Expected 4 arguments dynamic:setAngularFactor(x,y,z)");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	float z = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float y = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float x = lua_tonumber(L, -1);
	lua_pop(L, 1);

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	dynamic->body->setAngularFactor(btVector3(x, y, z));
	dynamic->activate();

	return 0;
}

static int LUA_dynamicGetVelocity(lua_State* L)
{
	scope("(LUA) dynamic:getVelocity");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 arguments dynamic:getVelocity()");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	btVector3 vel = dynamic->getVelocity();
	lua_pushnumber(L, vel.x());
	lua_pushnumber(L, vel.y());
	lua_pushnumber(L, vel.z());

	return 3;
}

static int LUA_dynamicSetPosition(lua_State* L)
{
	scope("(LUA) dynamic:setPosition");

	int args = lua_gettop(L);

	if (args != 4)
	{
		error("Expected 4 arguments dynamic:setPosition(x,y,z)");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	float z = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float y = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float x = lua_tonumber(L, -1);
	lua_pop(L, 1);

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	dynamic->setPosition(btVector3(x, y, z));
	dynamic->activate();

	return 0;
}

static int LUA_dynamicActivate(lua_State* L)
{
	scope("(LUA) dynamic:activate");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 argument dynamic:activate()");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	dynamic->activate();

	return 0;
}

static int LUA_dynamicGetPosition(lua_State* L)
{
	scope("(LUA) dynamic:getPosition");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 arguments dynamic:getPosition()");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	btVector3 vel = dynamic->getPosition();
	lua_pushnumber(L, vel.x());
	lua_pushnumber(L, vel.y());
	lua_pushnumber(L, vel.z());

	return 3;
}

static int LUA_getDynamicId(lua_State* L)
{
	scope("(LUA) getDynamicId");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 arguments getDynamicId(netId)");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	int id = lua_tointeger(L, -1);

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->find(id);

	if (!dynamic)
	{
		error("Invalid dynamic object id passed, does it exist?");
		return 0;
	}

	LUA_pd->dynamics->pushLua(L, dynamic);

	return 1;
}

static int LUA_getDynamicIdx(lua_State* L)
{
	scope("(LUA) getDynamicIdx");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 arguments getDynamicIdx(index)");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	int idx = lua_tointeger(L, -1);

	if(idx < 0 || idx >= LUA_pd->dynamics->size())
	{
		error("Invalid dynamic object index passed, size: " + std::to_string(LUA_pd->dynamics->size()) + ", index: " + std::to_string(idx));
		return 0;
	}

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->get(idx);

	LUA_pd->dynamics->pushLua(L, dynamic);

	return 1;
}

static int getNumDynamics(lua_State* L)
{
	scope("(LUA) getNumDynamics");

	int args = lua_gettop(L);

	if (args != 0)
	{
		error("Expected 0 arguments getNumDynamics()");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	lua_pushinteger(L, LUA_pd->dynamics->size());

	return 1;
}

static int LUA_dynamicSetAngularVelocity(lua_State* L)
{
	scope("(LUA) dynamic:setAngularVelocity");

	int args = lua_gettop(L);

	if (args != 4)
	{
		error("Expected 4 arguments dynamic:setAngularVelocity(x,y,z)");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	float z = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float y = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float x = lua_tonumber(L, -1);
	lua_pop(L, 1);

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	dynamic->setAngularVelocity(btVector3(x, y, z));
	dynamic->activate();

	return 0;
}

static int LUA_dynamicGetAngularVelocity(lua_State * L)
{
	scope("(LUA) dynamic:getAngularVelocity");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 arguments dynamic:getAngularVelocity()");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	btVector3 vel = dynamic->getAngularVelocity();
	lua_pushnumber(L, vel.x());
	lua_pushnumber(L, vel.y());
	lua_pushnumber(L, vel.z());

	return 3;
}

static int LUA_dynamicIsActive(lua_State* L)
{
	scope("(LUA) dynamic:isActive");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 arguments dynamic:isActive()");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	lua_pushboolean(L, dynamic->body->isActive());

	return 1;
}

static int LUA_dynamicSetGravity(lua_State* L)
{
	scope("(LUA) dynamic:setGravity");

	int args = lua_gettop(L);

	if (args != 4)
	{
		error("Expected 4 arguments dynamic:setGravity(x,y,z)");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	float z = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float y = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float x = lua_tonumber(L, -1);
	lua_pop(L, 1);

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	dynamic->body->setGravity(btVector3(x, y, z));
	dynamic->activate();

	return 0;
}

static int LUA_dynamicGetGravity(lua_State* L)
{
	scope("(LUA) dynamic:getGravity");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 arguments dynamic:getGravity()");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	btVector3 vel = dynamic->body->getGravity();
	lua_pushnumber(L, vel.x());
	lua_pushnumber(L, vel.y());
	lua_pushnumber(L, vel.z());

	return 3;
}

static int LUA_dynamicGetFriction(lua_State* L)
{
	scope("(LUA) dynamic:getFriction");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 arguments dynamic:getFriction()");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	lua_pushnumber(L, dynamic->body->getFriction());

	return 1;
}

static int LUA_dynamicGetRestitution(lua_State* L)
{
	scope("(LUA) dynamic:getRestitution");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 arguments dynamic:getRestitution()");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	lua_pushnumber(L, dynamic->body->getRestitution());

	return 1;
}

static int LUA_dynamicSetFriction(lua_State* L)
{
	scope("(LUA) dynamic:setFriction");

	int args = lua_gettop(L);

	if (args != 2)
	{
		error("Expected 2 arguments dynamic:setFriction(friction)");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	float friction = lua_tonumber(L, -1);
	lua_pop(L, 1);

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	friction = std::clamp(friction, 0.0f, 10.0f);

	dynamic->body->setFriction(friction);

	return 0;
}

static int LUA_dynamicSetRestitution(lua_State* L)
{
	scope("(LUA) dynamic:setRestitution");

	int args = lua_gettop(L);

	if (args != 2)
	{
		error("Expected 2 arguments dynamic:setRestitution(restitution)");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	float friction = lua_tonumber(L, -1);
	lua_pop(L, 1);

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	friction = std::clamp(friction, 0.0f, 10.0f);

	dynamic->body->setRestitution(friction);

	return 0;
}

static int LUA_dynamicGetRotation(lua_State* L)
{
	scope("(LUA) dynamic:getRotation");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 arguments dynamic:getRotation()");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	btQuaternion rot = dynamic->body->getWorldTransform().getRotation();
	lua_pushnumber(L, rot.w());
	lua_pushnumber(L, rot.x());
	lua_pushnumber(L, rot.y());
	lua_pushnumber(L, rot.z());

	return 4;
}

static int LUA_dynamicSetRotation(lua_State* L)
{
	scope("(LUA) dynamic:setRotation");

	int args = lua_gettop(L);

	if (args != 5)
	{
		error("Expected 5 arguments dynamic:setRotation(w,x,y,z)");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	float z = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float y = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float x = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float w = lua_tonumber(L, -1);
	lua_pop(L, 1);

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	btTransform t = dynamic->body->getWorldTransform();
	t.setRotation(btQuaternion(x, y, z, w));
	dynamic->body->setWorldTransform(t);
	dynamic->activate();

	return 0;
}

static int LUA_dynamicGetMass(lua_State* L)
{
	scope("(LUA) dynamic:getMass");

	int args = lua_gettop(L);

	if (args != 1)
	{
		error("Expected 1 arguments dynamic:getMass()");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	lua_pushnumber(L, dynamic->body->getMass());

	return 1;
}

static int LUA_dynamicSetMassProps(lua_State* L)
{
	scope("(LUA) dynamic:setMassProps");

	int args = lua_gettop(L);

	if (args != 5)
	{
		error("Expected 5 arguments dynamic:setMassProps(mass,centerX,centerY,centerZ)");
		return 0;
	}

	if (!LUA_pd->dynamics)
	{
		error("dynamics ObjHolder is null");
		return 0;
	}

	float z = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float y = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float x = lua_tonumber(L, -1);
	lua_pop(L, 1);
	float mass = lua_tonumber(L, -1);
	lua_pop(L, 1);

	std::shared_ptr<Dynamic> dynamic = LUA_pd->dynamics->popLua(L);

	if (!dynamic)
	{
		error("Invalid dynamic object passed, was it deleted already?");
		return 0;
	}

	dynamic->body->setMassProps(mass, btVector3(x, y, z));

	return 0;
}

luaL_Reg* getDynamicFunctions(lua_State *L)
{
	//Register dynamic global functions:
	lua_register(L, "createDynamic", LUA_createDynamic);
	lua_register(L, "getDynamicId", LUA_getDynamicId);
	lua_register(L, "getDynamicIdx", LUA_getDynamicIdx);
	lua_register(L, "getNumDynamics", getNumDynamics);

	//Create table of dynamic metatable functions:
	luaL_Reg* regs = new luaL_Reg[21];

	int iter = 0;
	regs[iter++] = { "destroy",     LUA_dynamicDestroy };
	regs[iter++] = { "getVelocity", LUA_dynamicGetVelocity };
	regs[iter++] = { "getPosition", LUA_dynamicGetPosition };
	regs[iter++] = { "setPosition", LUA_dynamicSetPosition };
	regs[iter++] = { "setVelocity", LUA_dynamicSetVelocity };
	regs[iter++] = { "setAngularVelocity", LUA_dynamicSetAngularVelocity };
	regs[iter++] = { "getAngularVelocity", LUA_dynamicGetAngularVelocity };
	regs[iter++] = { "setAngularFactor", LUA_dynamicSetAngularFactor };
	regs[iter++] = { "activate",    LUA_dynamicActivate };
	regs[iter++] = { "isActive",    LUA_dynamicIsActive };
	regs[iter++] = { "getGravity",    LUA_dynamicGetGravity};
	regs[iter++] = { "setGravity",    LUA_dynamicSetGravity };
	regs[iter++] = { "setRestitution",    LUA_dynamicSetRestitution };
	regs[iter++] = { "getRestitution",    LUA_dynamicGetRestitution };
	regs[iter++] = { "setFriction",    LUA_dynamicSetFriction };
	regs[iter++] = { "getFriction",    LUA_dynamicGetFriction };
	regs[iter++] = { "setRotation",    LUA_dynamicSetRotation };
	regs[iter++] = { "getRotation",    LUA_dynamicGetRotation };
	regs[iter++] = { "getMass",    LUA_dynamicGetMass };
	regs[iter++] = { "setMassProps",    LUA_dynamicSetMassProps };
	regs[iter++] = { NULL, NULL };

	return regs;
}



