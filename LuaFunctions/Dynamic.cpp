#include "Dynamic.h"

#include "../Networking/Server.h"
#include "../Networking/ObjHolder.h"
#include "../GameLoop/ServerProgramData.h"

luaL_Reg* getDynamicFunctions()
{
	luaL_Reg* regs = new luaL_Reg[1];

	int iter = 0;
	regs[iter++] = { NULL, NULL };

	return regs;
}
