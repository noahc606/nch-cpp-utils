#include "Log.h"
#include <SDL2/SDL.h>

int Log::logMode = LogModes::DEFAULT;
bool Log::logToFile = false;
bool Log::logDestroyed = false;


Log::Log(){}
Log::~Log(){}

void Log::throwException(std::string funcname, std::string format)
{
	error(funcname, format);
	throwException();
}

void Log::throwException()
{
    throw std::exception();
}

void Log::logString(std::string s)
{
	std::cout << s;
}

void Log::logSStream(std::stringstream& ss)
{
	logString(ss.str());
}