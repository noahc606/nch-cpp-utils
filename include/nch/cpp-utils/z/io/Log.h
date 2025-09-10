#pragma once
#include <iostream>
#include <memory>
#include <sstream>

namespace nch { class Log
{
public:
    /**/
    Log();
    virtual ~Log();
    /**/	
    template<typename ... T> static std::string getFormattedString(const std::string& format, T ... args) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wformat-security"

        int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...)+1; //Extra space for '\0'
        if(size_s<=0) {
            Log::warnv(__PRETTY_FUNCTION__, "printing unformatted string", "Error during formatting.");
            return format;
        }
        auto size = static_cast<size_t>(size_s);
        std::unique_ptr<char[]> buf(new char[size]);
        std::snprintf(buf.get(), size, format.c_str(), args ...);
        return std::string(buf.get(), buf.get()+size-1); //We don't want the '\0' inside

        #pragma GCC diagnostic pop
    }

    /* Normal logging (almost same as printf) */
    template<typename ... T> static void log(std::string format, T ... args) {

        if(enabledBrackets) {
            if(enabledColors) logString("\033[1;37m");
            logString("[  Log  ] ");
            if(enabledColors) logString("\033[1;0m");
        }
        if(enabledColors) logString("\033[0;37m");
        logString(getFormattedString(format+"\n", args ... ));
        if(enabledColors) logString("\033[1;0m");
    }
    template<typename ... T> static void log(std::string str) { log("%s", str.c_str()); }
    template<typename ... T> static void log() { log(""); }
    
    /* Debug (log only if debugging on) */
    template<typename ... T> static void debug(std::string format, T ... args) {
        #ifdef NDEBUG
            return;
        #endif
        if(enabledBrackets) {
            if(enabledColors) logString("\033[1;36m");
            logString("[ Debug ] ");
            if(enabledColors) logString("\033[1;0m");
        }
        if(enabledColors) logString("\033[0;36m");
        logString(getFormattedString(format+"\n", args ... ));
        if(enabledColors) logString("\033[1;0m");
    }


    /* Warning (log during bad program state) */
    template<typename ... T> static void warnv(std::string funcname, std::string resolution, std::string format, T ... args) {
        if(enabledBrackets) {
            if(enabledColors) logString("\033[1;33m");
            logString("[Warning] ");
            if(enabledColors) logString("\033[1;0m");
        }

        #ifndef NDEBUG
            if(enabledColors) logString("\033[0;35m");
            logString(funcname);
            if(enabledColors) logString("\033[1;0m");
            logString(" - ");
        #endif
        
        if(enabledColors) logString("\033[0;33m");
        logString(getFormattedString(format+", "+resolution+"\n", args ...));
        if(enabledColors) logString("\033[1;0m");
    }
    template<typename ... T> static void warn(std::string funcname, std::string format, T ... args) {
        warnv(funcname, "ignoring issue", format, args ...);
    }

    /* Error (log during invalid program state) */
    template<typename ... T> static void error(std::string funcname, std::string format, T ... args) {
        if(enabledBrackets) {
            if(enabledColors) logString("\033[1;31m");
            logString("[ ERROR ] ");
            if(enabledColors) logString("\033[1;0m");
        }
        
        #ifndef NDEBUG
            if(enabledColors) logString("\033[0;35m");
            logString(funcname);
            if(enabledColors) logString("\033[1;0m");
            logString(" - ");
        #endif

        if(enabledColors) logString("\033[0;31m");
        logString(getFormattedString(format+"!\n", args ...));
        if(enabledColors) logString("\033[1;0m");
    }
    template<typename ... T> static void errorv(std::string funcname, std::string errorOrigin, std::string format, T ... args) {
        if(enabledBrackets) {
            if(enabledColors) logString("\033[1;31m");
            logString("[ ERROR ] ");
            if(enabledColors) logString("\033[1;0m");
        }

        #ifndef NDEBUG
            if(enabledColors) logString("\033[0;35m");
            logString(funcname);
            if(enabledColors) logString("\033[1;0m");
            logString(" - ");
        #endif

        if(enabledColors) logString("\033[0;31m");
        logString(getFormattedString(errorOrigin+": "+format+"!\n", args ...));
        if(enabledColors) logString("\033[1;0m");
    }

    static void throwException(std::string funcname, std::string format);
    static void throwException();
    /**/

    static bool enabledBrackets;
    static bool enabledColors;
protected:

private:
	static void logString(std::string s);
	static void logSStream(std::stringstream& ss);

	static bool logToFile;
	static bool logDestroyed;
};
}