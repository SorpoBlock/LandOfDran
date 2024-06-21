#include "Logger.h"

std::function<void(std::string)> debug = Logger::debug;
std::function<void(std::string)> error = Logger::error;
std::function<void(std::string)> info = Logger::infoNewLine;
std::function<void(std::string)> addScope = Logger::addScope;
std::function<void()> leave = Logger::leave;
std::deque<loggerLine> Logger::storage;

Logger::Logger()
{
    //Target of last logStorageLines lines, though going over wouldn't cause crashes or anything
    //storage.reserve(logStorageLines);
}

Logger::~Logger()
{
    if (infoFileOpened)
        infoFile.close();
    if (errorFileOpened)
        errorFile.close();
}

void Logger::addScope(std::string text)
{
    //Add scope on top of the stack
    get().scopes.push_back(text);
}

void Logger::leave()
{
    if (get().scopes.size() < 1)
        return;
    //Remove last added scope from the stack
    get().scopes.erase(get().scopes.begin() + get().scopes.size() - 1);
}

void Logger::setDebug(bool useDebug)
{
    get().debugMode = useDebug;
}

Logger& Logger::get()
{
    static Logger instance;
    return instance;
}

Logger& Logger::operator<<(std::string text)
{
    if (!get().newLineNeeded)
    {
        if (Logger::get().infoFile.is_open())
            Logger::get().infoFile << format("", true);
        std::cout << format("", true);
    }

    if (Logger::get().infoFile.is_open())
        Logger::get().infoFile << text;
    std::cout << text;
    get().newLineNeeded = true;
    return get();
}

Logger& Logger::operator<<(int text)
{
    get() << std::to_string(text);
    return get();
}

Logger& Logger::operator<<(float text)
{
    get() << std::to_string(text);
    return get();
}

std::string Logger::format(std::string text, bool noLine, bool noHeader)
{
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y/%m/%d - %X", &tstruct);

    std::string ret = "";
    if (!noHeader)
    {
        ret = std::string(buf) + " - ";
        for (unsigned int a = 0; a < get().scopes.size(); a++)
        {
            std::string scope = get().scopes[a];
            if (scope.length() > 0)
                ret += scope + " - ";
        }
    }
    ret += text;
    if (!noLine)
        ret += "\n";
    return ret;
}

//This probably isn't how you're supposed to implement a singleton...
bool Logger::setInfoFile(std::string path)
{
    std::filesystem::create_directories(std::filesystem::path(path.c_str()).parent_path());

    if (Logger::get().infoFile.is_open())
        Logger::get().infoFile.close();
    Logger::get().infoFileOpened = false;
    Logger::get().infoFile.open(path.c_str(), std::ios_base::app);
    if (Logger::get().infoFile.is_open())
    {
        Logger::get().infoFileOpened = true;
        return true;
    }
    else
    {
        std::cout << "Could not open log file " << path << "\n";
        return false;
    }
}

bool Logger::setErrorFile(std::string path)
{
    std::filesystem::create_directories(std::filesystem::path(path.c_str()).parent_path());

    if (Logger::get().errorFile.is_open())
        Logger::get().errorFile.close();
    Logger::get().errorFileOpened = false;
    Logger::get().errorFile.open(path.c_str(), std::ios_base::app);
    if (Logger::get().errorFile.is_open())
    {
        Logger::get().errorFileOpened = true;
        return true;
    }
    else
    {
        std::cout << "Could not open error file " << path << "\n";
        return false;
    }
}

void Logger::error(std::string text)
{
    if (storage.size() >= logStorageLines)
        storage.pop_back();
    storage.push_front({ true,false,text });

    if (Logger::get().errorFile.is_open())
        Logger::get().errorFile << Logger::format(text);

    info("ERROR - " + text);
}

void Logger::infoNewLine(std::string text)
{
    if (storage.size() >= logStorageLines)
        storage.pop_back();
    storage.push_front({false,false,text});

    info(text);
}

void Logger::info(std::string text, bool noLine)
{
    if (get().newLineNeeded)
    {
        get().newLineNeeded = false;
        if (Logger::get().infoFile.is_open())
            Logger::get().infoFile << "\n";
        std::cout << "\n";
    }

    if (Logger::get().infoFile.is_open())
        Logger::get().infoFile << Logger::format(text, noLine);

    std::cout << Logger::format(text, noLine);
}

void Logger::debug(std::string text)
{
    if (!Logger::get().debugMode)
        return;

    if (storage.size() >= logStorageLines)
        storage.pop_back();
    storage.push_front({ false,true,text });

    info(text);
}

const std::deque<loggerLine> * const Logger::getStorage()
{
    return &storage;
}
