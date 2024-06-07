#pragma once

#include "../LandOfDran.h"

/*
    You can use the macro scope("text here") at the start of important functions
    It will help create a readable backtrace for logged messages
    Example:
    void myFunc()
    {
        scope("myFunc");
        info("Hey there");
    }
*/

/*
    Basic logger that logs to console as well as text files
    Three levels of logging:
    Error - For bad stuff, gets its own file
    Info - For anything useful but not bad
    Debug - Optional verbose logging
*/
class Logger
{
    public:
        //Pushes and pops scopes from the stack for logging:
        static void addScope(std::string text);
        //Don't call this, just use the scope(str) macro
        static void leave();

        //Three levels of logging:
        static void info(std::string text, bool noLine = false);
        static void infoNewLine(std::string text);
        static void error(std::string text);
        static void debug(std::string text); //writes to infoFile only if debugMode = true

        //Should we ignore debug logging calls
        static void setDebug(bool useDebug);

        //Stream operators, but I don't use them much
        Logger& operator<<(std::string);
        Logger& operator<<(int);
        Logger& operator<<(float);

        //Set which file to log info and debug messages to, automatically creates directories
        static bool setInfoFile(std::string path);
        //Set which file to log only error messages to, automatically creates directories
        static bool setErrorFile(std::string path);

        static Logger& get();

        //Make sure there can't ever be more than one logger at a time
        Logger(Logger const&) = delete;
        Logger &operator=(Logger const&) = delete;

    private:
        static std::string format(std::string text, bool noLine = false, bool noHeader = false);

        std::vector<std::string> scopes;
        bool newLineNeeded = false;
        bool debugMode = false;
        bool infoFileOpened = false;
        bool errorFileOpened = false;
        std::ofstream infoFile;
        std::ofstream errorFile;
         
        Logger() {}
        ~Logger();
};

extern std::function<void(std::string)> info;
extern std::function<void(std::string)> debug;
extern std::function<void(std::string)> error; 
extern std::function<void(std::string)> addScope;
extern std::function<void()> leave;

//The whole point of this, is just to call global leave() when at the end of a given scope:
struct scopeLogger
{
    scopeLogger() {};
    ~scopeLogger() { leave(); };
};

//Adds a scope name to the stack, and removes it when the current scope is left
#define scope(n) addScope(n); scopeLogger tempScopeName;