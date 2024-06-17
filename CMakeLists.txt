# CMakeList.txt : CMake project for LandOfDran, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project (LandOfDran)

include(C:/Users/Syerjchep/vcpkg/scripts/buildsystems/vcpkg.cmake)
find_package(ZLIB REQUIRED)
find_package(glew CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenAL CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

# Add source to this project's executable.
add_executable (LandOfDran 
"LandOfDran.cpp" 
"LandOfDran.h" 
"Utility/Logger.h" 
"Utility/Logger.cpp" 
"Utility/SettingManager.h" 
"Utility/SettingManager.cpp" 
"Utility/StringFunctions.h" 
"Utility/StringFunctions.cpp" 
"Utility/DefaultPreferences.cpp" 
"Utility/DefaultPreferences.h" 
"Utility/DefaultPreferences.cpp" 
"Utility/GlobalStartup.h" 
"Utility/GlobalStartup.cpp"
"Utility/FileFunctions.h" 
"Utility/FileFunctions.cpp"
#"Utility/UserInterfaceFunctions.h" 
#"Utility/UserInterfaceFunctions.cpp"

"External/CRC.h" 
"External/stb_image.h" 

"External/Imgui/imgui.h"
"External/Imgui/imgui.cpp"
"External/Imgui/imgui_impl_opengl3.cpp"
"External/Imgui/imgui_impl_opengl3.h"
"External/Imgui/imgui_impl_sdl2.cpp"
"External/Imgui/imgui_impl_sdl2.h"
"External/Imgui/imgui_stdlib.cpp"
"External/Imgui/imgui_stdlib.h"
"External/Imgui/imgui_tables.cpp"
"External/Imgui/imgui_widgets.cpp"
"External/Imgui/imstb_rectpack.h"
"External/Imgui/imstb_textedit.h"
"External/Imgui/imstb_truetype.h"
"External/Imgui/imgui_internal.h"
"External/Imgui/imgui_draw.cpp"
#"External/Imgui/imconfig.h"
 
"Graphics/RenderContext.h"
"Graphics/RenderContext.cpp" 
"Graphics/Shader.h" 
"Graphics/Shader.cpp" 
"Graphics/Program.h"  
"Graphics/Program.cpp"  
"Graphics/ShaderSpecification.h" 
"Graphics/ShaderSpecification.cpp" 
"Graphics/GraphicsHelpers.h" 
"Graphics/GraphicsHelpers.cpp" 
"Graphics/Texture.h" 
"Graphics/Texture.cpp"
"Graphics/Material.h" 
"Graphics/Material.cpp" 
"Graphics/Mesh.h" 
"Graphics/Mesh.cpp" 
"Interface/InputMap.h"  
"Interface/InputMap.cpp" 
"Graphics/PlayerCamera.h" 
"Graphics/PlayerCamera.cpp" 
)

target_link_libraries(LandOfDran PRIVATE ZLIB::ZLIB)
target_link_libraries(LandOfDran PRIVATE SDL2::SDL2 SDL2::SDL2main)
target_link_libraries(LandOfDran PRIVATE OpenAL::OpenAL)
target_link_libraries(LandOfDran PRIVATE GLEW::glew)
target_link_libraries(LandOfDran PRIVATE OpenGL::GL)
target_link_libraries(LandOfDran PRIVATE CURL::libcurl)
target_link_libraries(LandOfDran PRIVATE ${BULLET_LIBRARIES})
target_link_libraries(LandOfDran PRIVATE assimp::assimp)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET LandOfDran PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.

