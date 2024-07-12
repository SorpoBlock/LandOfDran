#include "InputMap.h"

std::string GetInputCommandString(InputCommand command)
{
    switch (command)
    {
        case NoCommand: return "No command";
        case EndOfCommands: return "Error";
        case WalkForward: return "Walk Forward";
        case WalkBackward: return "Walk Backward";
        case WalkLeft: return "Walk Left";
        case WalkRight: return "Walk Right";
        case MouseLock: return "Toggle Mouse Lock";
        case OpenOptionsMenu: return "Open Settings";
        case OpenDebugWindow: return "Open Debug Window";
        case OpenChatWindow: return "Open Chat Window";
        case FirstThirdPerson: return "Switch 1st/3rd person";
        case Jump: return "Jump";
        default: return "Other error";
    }
}

void InputMap::resetKeyStates()
{
    for (int a = 0; a < InputCommand::EndOfCommands; a++)
    {
        keyPolled[a] = false;
        keyToProcess[a] = false;
        keyPressed[a] = false;
    }
}

void InputMap::setPreferences(std::shared_ptr<SettingManager> settings)
{
    for (unsigned int a = 1; a < InputCommand::EndOfCommands; a++)
        settings->addInt("keybinds/" + std::to_string(a), keyForCommand[a]);
}

InputMap::InputMap(std::shared_ptr<SettingManager> settings)
{
    if (settings)
    {
        //Load key binds from file
        for (unsigned int a = 1; a < InputCommand::EndOfCommands; a++)
        {
            const PreferencePair* pref = settings->getPreference("keybinds/" + std::to_string(a));
            if (!pref)
                continue;

            keyForCommand[a] = (SDL_Scancode)settings->getInt("keybinds/" + std::to_string(a));
        }

        //Default key bindings here:
        //Forget a key and it'll crash!
        bindKey(WalkForward, SDL_SCANCODE_W);
        bindKey(WalkBackward, SDL_SCANCODE_S);
        bindKey(WalkRight, SDL_SCANCODE_D);
        bindKey(WalkLeft, SDL_SCANCODE_A);
        bindKey(MouseLock, SDL_SCANCODE_M);
        bindKey(OpenOptionsMenu, SDL_SCANCODE_O);
        bindKey(OpenDebugWindow, SDL_SCANCODE_GRAVE);
        bindKey(OpenChatWindow, SDL_SCANCODE_C);
        bindKey(FirstThirdPerson, SDL_SCANCODE_TAB);
        bindKey(Jump, SDL_SCANCODE_SPACE);

        for (unsigned int a = 1; a < InputCommand::EndOfCommands; a++)
            settings->addInt("keybinds/" + std::to_string(a), keyForCommand[a]);
    }

    resetKeyStates();
}

SDL_Scancode InputMap::getKeyBind(InputCommand command) const
{
    if (command == NoCommand || command == InputCommand::EndOfCommands)
        return SDL_NUM_SCANCODES;
    return keyForCommand[command];
}

void InputMap::bindKey(InputCommand command, SDL_Scancode key)
{
    if (command == NoCommand || command == InputCommand::EndOfCommands)
        return;
    keyForCommand[command] = key;
}

void InputMap::handleInput(SDL_Event& event)
{
    if (event.type != SDL_KEYDOWN && event.type != SDL_KEYUP)
        return;

    if (supressed)
        return;

    for (unsigned int a = 1; a < InputCommand::EndOfCommands; a++)
    {
        if (keyForCommand[a] == event.key.keysym.scancode)
        {
            if (event.type == SDL_KEYDOWN)
            {
                keyPressed[a] = true;
                if (!keyToProcess[a])
                {
                    keyToProcess[a] = true;
                    keyPolled[a] = false;
                }
            }
            else
            {
                keyPressed[a] = false;
                if (keyPolled[a])
                {
                    keyToProcess[a] = false;
                    keyPolled[a] = false;
                }
            }

            break;
        }
    }
}

bool InputMap::pollCommand(InputCommand command)
{
    if (supressed)
        return false;

    if (command == InputCommand::EndOfCommands || command == NoCommand)
        return false;

    if (keyToProcess[command])
    {
        if (!keyPolled[command])
        {
            if (!keyPressed[command])
            {
                keyPolled[command] = false;
                keyToProcess[command] = false;
                keyPressed[command] = false;
            }
            else
                keyPolled[command] = true;
            return true;
        }
    }

    return false;
}


bool InputMap::isCommandKeydown(InputCommand command) const
{
    if (supressed)
        return false;

    if (keystates == nullptr)
    {
        scope("inputMap::commandKeyDown");
        error("Need to call getKeyStates first");
        return false;
    }

    if (command == InputCommand::EndOfCommands || command == InputCommand::NoCommand)
        return false;

    SDL_Scancode key = keyForCommand[command];
    return keystates[key];
}
