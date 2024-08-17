#include "Log.h"
#include <SDL2/SDL.h>

int NCH_Log::logMode = LogModes::DEFAULT;
bool NCH_Log::logToFile = false;
bool NCH_Log::logDestroyed = false;


NCH_Log::NCH_Log(){}
NCH_Log::~NCH_Log(){}

void NCH_Log::throwException(std::string funcname, std::string format)
{
	error(funcname, format);
	throwException();
}

void NCH_Log::throwException()
{
    throw std::exception();
}

void NCH_Log::logString(std::string s)
{
	std::cout << s;
}

void NCH_Log::logSStream(std::stringstream& ss)
{
	logString(ss.str());
}