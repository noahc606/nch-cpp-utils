#include "Log.h"
#include <SDL2/SDL.h>

using namespace nch;

bool Log::enabledBrackets = true;
bool Log::enabledColors = false;

bool Log::logToFile = false;
bool Log::logDestroyed = false;
std::recursive_mutex Log::rmtx;
int64_t Log::suppressionsLeft = 0;

std::function<void(const std::string&)> Log::sink_;

Log::Log(){}
Log::~Log(){}

void Log::throwException(std::string funcname, std::string format) {
	std::lock_guard<std::recursive_mutex> lock(rmtx);
	error(funcname, format);
	throwException();
}

void Log::throwException() {
	std::lock_guard<std::recursive_mutex> lock(rmtx);
    throw std::exception();
}

void Log::logString(std::string s) {
	std::lock_guard<std::recursive_mutex> lock(rmtx);

	std::cout << s;
	if(sink_) sink_(s);
}

void Log::setSink(std::function<void(const std::string&)> s) {
	std::lock_guard<std::recursive_mutex> lock(rmtx);
	sink_ = std::move(s);
}
void Log::suppressNext(int64_t amount) {
	std::lock_guard<std::recursive_mutex> lock(rmtx);
	suppressionsLeft += amount;
}
void Log::suppressClear() {
	std::lock_guard<std::recursive_mutex> lock(rmtx);
	suppressionsLeft = 0;
}
bool Log::consumeSuppression() {
	std::lock_guard<std::recursive_mutex> lock(rmtx);
	if(suppressionsLeft<=0) {
		suppressionsLeft = 0;
		return false;
	}
	suppressionsLeft--;
	return true;
}

void Log::logSStream(std::stringstream& ss) {
	std::lock_guard<std::recursive_mutex> lock(rmtx);
	logString(ss.str());
}