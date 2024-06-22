﻿// LandOfDran.h : Mostly external includes that can be included anywhere and not mess with anything

#include "enet/enet.h"
#include <ctype.h>
#include <iostream>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include <functional>
#include <time.h>
#include <string>
#include <filesystem>
#include <iterator>
#include <numeric>
#include <string_view>
#include <complex>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <map>
#include <stack>
#include <memory>
#include "Utility/ExecutableArguments.h"
#include "Utility/StringFunctions.h"
#include "Utility/Logger.h"
#include "Utility/FileFunctions.h"
#include "Utility/SettingManager.h"
#include "Networking/PacketEnums.h"
#include "Graphics/GraphicsHelpers.h"

//Used in the titlebar for the window, and for making sure client and server version match in mutliplayer
#define GAME_VERSION 20

//Default port for land of dran
#define DEFAULT_PORT 8765
