// LandOfDran.h : Mostly external includes that can be included anywhere and not mess with anything

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
#include "Utility/StringFunctions.h"
#include "Utility/Logger.h"
#include "Utility/FileFunctions.h"
#include "Graphics/GraphicsHelpers.h"
#include <map>
#include <stack>
#include <memory>

//Used in the titlebar for the window, and for making sure client and server version match in mutliplayer
#define GAME_VERSION 20
