#pragma once

#include "../JoinedClient.h"
#include "../../GameLoop/ServerProgramData.h"

//No need for a HeldPacket abstract base class here, server doesn't really have any 
//scenarios where a packet couldn't be 'applied' now but could be applied later

void applyConnectionRequest(JoinedClient const * const source,ENetPacket const * const packet, const ServerProgramData& pd);
