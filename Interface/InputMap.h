#pragma once

#include "../LandOfDran.h"
#include "../Utility/SettingManager.h"

/*
	A list of commands that can be bound to keys 
*/
enum InputCommand
{
	NoCommand = 0,
	WalkForward = 1,
	WalkBackward = 2,
	WalkLeft = 3,
	WalkRight = 4,
	MouseLock = 5,
    OptionsMenu = 6,
    CloseWindow = 7,
    EndOfCommands = 8
};

//For user interface
std::string GetInputCommandString(InputCommand command);

/*
    Basically this is an intermediate layer that allows the player to make custom key binds
    It uses the SDL event polling frame work, filters it through a preference file, and provides its own event polling framework
    Call InputMap::handleInput in the start of your SDL_PollEvent loop
*/
class InputMap
{
    //One possible command for each key
    SDL_Scancode keyForCommand[InputCommand::EndOfCommands];

    //Is there a previous key press that still needs to be handled?
    bool keyToProcess[InputCommand::EndOfCommands];

    //We can't process a new instance of a command until the key has been released, and the last instance has been processed:
    bool keyPressed[InputCommand::EndOfCommands];
    bool keyPolled[InputCommand::EndOfCommands];

    public:
    //Data directly from SDL_GetKeyboardState, you have to read it in
    const Uint8* keystates = nullptr;

    //We're typing in a text box or something where generally we don't want any keypresses to do anything in-game
    bool supressed = false;

    //Call this during a loop like while(SDL_PollEvent(&e))
    void handleInput(SDL_Event& event);

    //Returns true if a full key press occured on this key since the last poll
    bool pollCommand(InputCommand command);

    //Is the key for this command *currently* down, we don't care about full key presses or anything here
    bool isCommandKeydown(InputCommand command) const;
    
    //Sets what key is bound to what command
    void bindKey(InputCommand command, SDL_Scancode key);

    //Clears any currently waiting commands to be polled
    void resetKeyStates();

    //Loads key binds from a preference file if one is given
    InputMap(std::shared_ptr<SettingManager> settings);
};