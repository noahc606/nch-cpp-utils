#pragma once
#include <iostream>
#include <memory>
#include <sstream>

namespace nch { class Log
{
public:
    enum LogModes {
        DEFAULT, DEBUGGING
    };

    /**/
    Log();
    virtual ~Log();
    /**/	
    template<typename ... T> static std::string getFormattedString(const std::string& format, T ... args) {
        int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
        if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
        auto size = static_cast<size_t>( size_s );
        std::unique_ptr<char[]> buf( new char[ size ] );
        std::snprintf( buf.get(), size, format.c_str(), args ... );
        return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
    }

    /* Normal logging (almost same as printf) */
    template<typename ... T> static void log(std::string format, T ... args) {
	    logString(getFormattedString("[  Log  ] "+format+"\n", args ... ));
    }
    
    /* Debug (log only if debugging OR troubleshooting is on) */
    template<typename ... T> static void debug(std::string format, T ... args) {
        if(logMode==LogModes::DEBUGGING) {
            logString(getFormattedString("[ Debug ] "+format+"\n", args ...));
        }
    }


    /* Warning (log during bad program state) */
    //Verbose
    template<typename ... T> static void warnv(std::string funcname, std::string resolution, std::string format, T ... args) {
        logString(getFormattedString("[Warning] "+funcname+" - "+format+", "+resolution+"\n", args ...));
    }
    //Normal warning
    template<typename ... T> static void warn (std::string funcname, std::string format, T ... args) {
        warnv(funcname, "ignoring issue", format, args ...);
    }

    /* Error (log during invalid program state) */
    //Verbose char* error
    template<typename ... T> static void errorv(std::string funcname, const char *error, std::string format, T ... args) {
        logString(getFormattedString("[ ERROR ] "+funcname+" - "+format+": "+error+"!\n", args ...));
    }
    //Verbose string error
    template<typename ... T> static void errorv(std::string funcname, std::string error, std::string format, T ... args) {
        errorv(funcname, error.c_str(), format, args ...);
    }
    //Normal error
    template<typename ... T> static void error(std::string funcname, std::string format, T ... args) {
    	logString(getFormattedString("[ ERROR ] "+funcname+" - "+format+"!\n", args ...));
    }

    static void throwException(std::string funcname, std::string format);
    static void throwException();
    /**/

    static int logMode;

protected:

private:
	static void logString(std::string s);
	static void logSStream(std::stringstream& ss);

	static bool logToFile;
	static bool logDestroyed;
};
}