#ifndef __BL_LOG_HPP__
#define __BL_LOG_HPP__

#ifdef _MSC_VER
/* MS VS complains following:
 *../bllibs/c++/include\bl/logger/log.hpp(208) : warning C4251: 'bl::logger::Logger::mux' : class 'boost::recursive_mutex' needs to have dll-interface to be used by clients of class 'bl::logger::Logger'
 *       v:/boost/boost_1_34_1\boost/thread/recursive_mutex.hpp(33) : see declaration of 'boost::recursive_mutex'
 *
 * Nobody cares and few knows why. So I will disable this warning, so it won't pollute output
 * If you know proper fix for that, please implement it. Thanks. (translation to English: this will never happen)
 */
#pragma warning(disable: 4251)
 
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

//#define _CRT_NOFORCE_MANIFEST
//#include <direct.h>
#if (_MSC_VER < 1600)
typedef signed long long int64_t;
typedef signed long int32_t;
typedef signed short int16_t;
typedef signed char int8_t;
typedef unsigned long long uint64_t;
typedef unsigned long uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
#endif

// make windows accept POSIX strcasecmp()
#define strcasecmp(arg1,arg2) _stricmp(arg1,arg2)
#define strncasecmp(arg1,arg2,arg3) _strnicmp(arg1,arg2,arg3)

#else

#include <sys/stat.h>
#include <sys/types.h>
// windows mkdir() only takes 1 arg
#define _mkdir(arg) mkdir(arg, 0777)
#endif

#ifdef WIN32
#ifndef BL_LOG_NO_DLL
#define LOG_DECLSPEC __declspec(dllexport)
#else
#define LOG_DECLSPEC
#endif
#else
#define LOG_DECLSPEC
#endif

#include <string>
#include <iostream>
#include <ostream>
#include <fstream>
#include <sstream>

#ifdef _MSC_VER
// Disable a harmless warning from asio due to a since-fixed inconsistency between MSVC and other compilers
#pragma warning(disable:4244)
#pragma warning(disable:4512)
#pragma warning(disable:4127)
#endif
#include <boost/thread/recursive_mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#ifdef _MSC_VER
#pragma warning(default:4127)
#pragma warning(default:4512)
#pragma warning(default:4244)
#endif

#include <stdarg.h>
#include <time.h>

#include "safequeue.hpp"



/*
 *
 The logger is initialized by default. If you want to change the parameters
 it gets, call the LogSetup(...) macro before calling one of the logging methods.

 =========================================================================
 Targets or Sinks:

 The logger has three different sinks or targets where the log messages
 will be delivered:

 1) print (stderr)

 2) Internal memory Queue
 This is accessible from code and it could be used for example
 to create a error GUI

 3) File

 All of these have own log level, which controls which messages are
 actually logged.

 =========================================================================
 Use the following macros instead of the logger class methods; these will
 provide the line number and function name where the logging occurred.

 bl_log_debug(msg)
 bl_log_info(msg)
 bl_log_warn(msg)
 bl_log_error(msg)
 bl_log_fatal(msg)

 Examples:

 bl_log_fatal("Foo" << "bar: " << 42);

 Also lower level interface is available:

 Examples: Log("Hello %s", "world");
 LogPri(ERR, "Some error happened: %s", "substring");
 LogStream() << "hello world";
 LogStreamPri(ERR) << "Some error happened";

 Note that newlines are implicit.

 =========================================================================
 If you want to build up output in a loop or something, try this, since
 newlines are implicit:

 std::ostringstream str;
 str << "something ";
 while(1)
 {
 str << "something else ";
 break;
 }
 LogStream() << str.str();

 =========================================================================
 Threading:
 The logger should be threadsafe. The message queue has the
 same limitations as the safequeue.

 =========================================================================
 Message Queue:

 The logger now also maintains a fixed-length message queue
 containing warnings and errors. This allows one thread
 to perform notification of errors happening in other threads.
 It can be safely ignored if not needed.

*/

#ifndef __GNUC__
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define getcwd _getcwd
#endif

namespace bl
{
    namespace logger
    {

        class DualStream;

        class LOG_DECLSPEC Logger
        {
            friend class DualStream;

        public:
            enum LogLevel
            {
                eDebug=0,
                eInfo,
                eWarning,
                eError,
                eFatal,

                eNone // This has to be the last one (it turns logging off)
            };

            struct LOG_DECLSPEC Message
            {
                LogLevel priority;
                std::string message;
            };

            static Logger *getLogger(void);
            static void killLogger(void);

            /**
             *
             * @param progname, name of program (in log files)
             * @param logdir, base directory where to put log files
             * @param size_max, maximun line count in one log file, 0 to disable rotation is no longer supported
             * @param print_level, log level to print stderr
             * @param file_level, log level to log to file
             * @param queue_level, log level to log to internal queue
             */
            static void setup(std::string program_name="logger",
                              std::string log_dir = "",
                              int size_max = Logger::default_size_max,
                              LogLevel print_level = eInfo,
                              LogLevel file_level = eDebug,
                              LogLevel queue_level = eWarning,
                              bool quiet_mode = false,
                              std::string header = "No header defined\n");

            /**
             * Set Max log file count for log rotation
             *
             * @param pi_maxLogCount, this is N total files, e.g. N-1 numbered, and one plain .log-file
             */
            static void setMaxLogFileCount(const int pi_maxLogCount);

            Logger()
            {
                orig_time = NULL;
                log_num = -1;
                openLog();
            }

            ~Logger(void)
            {
                output.close();
            }

            static void fileSettings(const int &file_lines, const int &max_files);

            void log(const std::string &file, const std::string &func, int line, std::string message);
            void log(const char *file, const char *func, std::string message, int line, ...);
            void log(const char *file, const char *func, LogLevel priority, std::string message, int line, ...);

            DualStream getStream(const std::string &file, const std::string &func, int line);
            DualStream getStream(const char *file, const char *func, int line);
            DualStream getStream(const char *file, const char *func, int line, LogLevel priority);
            DualStream getStream(const char *file, const char *func, int line, LogLevel priority, const char *colorStr);
            void logToStream(LogLevel priority, unsigned char *msg, const int msgSize, const bool pretty);
            Message getMessage() { return msgQueue.pop(); };
            bool messageQueueEmpty() { return msgQueue.empty(); };
            size_t messageQueueSize() { return msgQueue.size(); };

            // convert any non-printable strings to \xx, returning the converted string
            static std::string convertNonPrintables(std::string input);

            // Returns the lowest log level
            LogLevel getLogLevel() const;

            std::string getLogFilename(int num);

        private:
            //static boost::shared_ptr<boost::recursive_mutex> mux;
            static boost::recursive_mutex mux;
            static Logger* instance;

            static std::string progname;

            static std::string logdir;

            static std::string header;
            /*
             * Max log file count for log rotation, this is N+1+1
             * e.g. N+1 numbered log files, and one plain .log file.
             */
            static int max_log_file_count;


            static LogLevel print_level;
            static LogLevel file_level;
            static LogLevel queue_level;

            void enqueue(LogLevel priority, const std::string &message);
            const static size_t MAX_QUEUE_SIZE = 100;

            SafeQueue < Message > msgQueue;

            void rotate();
            void openLog();


            std::ofstream output;
            static size_t size_max;
            const static size_t default_size_max;

            char *orig_time;
            int log_num;

            static bool quiet_mode;
        };

        /*
         * DualStream & timestamps
         *
         * We create timestamps while we are constructing the DualStream instance.
         * The sunny side of this is that we have more correct and accurate timestamps
         * (we are not doing it before log rotation, accuiring mutexes etc.)
         * The flip side is that in theory we could have an younger timestamp before
         * an older timestamp in the log files.
         *
         * However, even in this case the timestamp reflects more correctly when the
         * log line/event happend in the originating code.  This functionality is
         * very important for Locution.
         */
        class LOG_DECLSPEC DualStream : public std::ostringstream
        {
        public:

#ifdef WIN32
// a local utility macro to strip slashes from a pathname
#define BL_LOG_STRIP_SLASHES(msg) (strrchr(msg, '\\') == NULL ? msg : strrchr(msg, '\\') + 1)
#define CONSOLE_COLOR_STR ""
#else
#define BL_LOG_STRIP_SLASHES(msg) (strrchr(msg, '/') == NULL ? msg : strrchr(msg, '/') + 1)
#define CONSOLE_COLOR_STR "\e[0m"
#endif

            DualStream(Logger &log, const char *pfile, const char *pfunc, int pline,
                       Logger::LogLevel ppriority, const char *pcolorStr = CONSOLE_COLOR_STR )
                : str(NULL),
                logger(log),
                m_ts(boost::posix_time::microsec_clock::local_time()),
                file(BL_LOG_STRIP_SLASHES(pfile)),
                func(pfunc),
                line(pline),
                priority(ppriority),
                colorStr(pcolorStr) { }

            DualStream(Logger &log, const std::string &pfile, const std::string &pfunc, int pline,
                       Logger::LogLevel ppriority, const char *pcolorStr = CONSOLE_COLOR_STR)
                : str(NULL),
                logger(log),
                m_ts(boost::posix_time::microsec_clock::local_time()),
                file(BL_LOG_STRIP_SLASHES(pfile.c_str())),
                func(pfunc.c_str()),
                line(pline),
                priority(ppriority),
                colorStr(pcolorStr) { }

            ~DualStream(void) { flush(); }
            DualStream(const DualStream &d)
                : str(d.str),
                logger(d.logger),
                m_ts(d.m_ts),
                file(BL_LOG_STRIP_SLASHES(d.file)),
                func(d.func),
                line(d.line),
                priority(d.priority),
                colorStr(d.colorStr) { }

#undef BL_LOG_STRIP_SLAHES

            template < typename T >
                DualStream &operator<<(const T &t)
            {
                if(!str)
                {
                    str = new std::ostringstream;
                }
                *str << t;
                return *this;
            }

            void flush(void);

        private:
            std::ostringstream *str;
            Logger &logger;
            boost::posix_time::ptime m_ts;
            const char *file;
            const char *func;
            int line;
            Logger::LogLevel priority;
            const char *colorStr;
        };


        template < typename T >
        DualStream operator<<(Logger &l, const T &t)
        {
            return l.getStream("NO FILE", "NO FUNC", -1);
        }


/*
 * Define a wrapper class for exporting to python.
 *
 * This way we can have a simple export with none of the issues
 * relating to function overloading or calling statics, and
 * not have to change anything in the logger.
 */
        class LOG_DECLSPEC PyLogger
        {
        public:
            void setup(std::string progname, int size_max, Logger::LogLevel print_level,
                       Logger::LogLevel file_level, std::string logdir, std::string header)
            {
                Logger::setup(progname, logdir, size_max, print_level, file_level, Logger::eWarning, false, header);
            }

            void log(const std::string &file, const std::string &func, Logger::LogLevel priority,
                     std::string message, int line)
            {
                Logger::getLogger()->getStream(file.c_str(), func.c_str(), line, priority) << message;
            }

            void setMaxLogFileCount(const int pi_maxLogCount)
            {
                Logger::setMaxLogFileCount(pi_maxLogCount);
            }

            Logger::Message getMessage() { return Logger::getLogger()->getMessage(); };

            bool messageQueueEmpty() { return Logger::getLogger()->messageQueueEmpty(); };
            size_t messageQueueSize() { return Logger::getLogger()->messageQueueSize(); };
        } ;

// these macros have to be below the class or the enum definition gets screwed up
#define BL_LOG_DEBUG  bl::logger::Logger::eDebug
#define BL_LOG_INFO   bl::logger::Logger::eInfo
#define BL_LOG_WARN   bl::logger::Logger::eWarning
#define BL_LOG_ERR    bl::logger::Logger::eError
#define BL_LOG_FATAL  bl::logger::Logger::eFatal

#define BLACK "\e[30m"
#define RED "\e[31m"
#define GREEN "\e[32m"
#define YELLOW "\e[33m"
#define BLUE "\e[34m"
#define MAGENTA "\e[35m"
#define CYAN "\e[36m"
#define WHITE "\e[37m"
#define NO_COLOR "\e[0m"

#define BLACK_ON_RED "\e[30;41m"
#define BLACK_ON_YELLOW "\e[30;43m"

#define LogStream() bl::logger::Logger::getLogger()->getStream(__FILE__, __FUNCTION__, __LINE__)
#define LogStreamPri(priority) bl::logger::Logger::getLogger()->getStream(__FILE__, __FUNCTION__, __LINE__, priority)


#define LogStreamPriColor(priority, colorStr)                           \
    bl::logger::Logger::getLogger()->getStream(__FILE__, __FUNCTION__, __LINE__, priority, colorStr)

#define LogStreamDebug() LogStreamPriColor(BL_LOG_DEBUG, GREEN)
#define LogStreamInfo() LogStreamPriColor(BL_LOG_INFO, MAGENTA)
#define LogStreamWarn() LogStreamPriColor(BL_LOG_WARN, YELLOW)
#define LogStreamErr() LogStreamPriColor(BL_LOG_ERR, RED)
#define LogStreamFatal() LogStreamPriColor(BL_LOG_FATAL, BLACK_ON_RED)
#define LogStreamTest() LogStreamPriColor(BL_LOG_INFO, CYAN)
#define LogStreamTime() LogStreamPriColor(BL_LOG_INFO, BLACK_ON_YELLOW)
#define LogStreamCommand() LogStreamPriColor(BL_LOG_INFO, BLUE)

#define bl_log(pri, msg)                                                \
    do {                                                            \
            if (bl::logger::Logger::getLogger()->getLogLevel() <= pri) { \
                bl::logger::Logger::getLogger()->getStream(__FILE__, __FUNCTION__, __LINE__, pri) << msg; \
            }                                                           \
    } while(0)

#define bl_log_ascii_pri(pri, msg, size, pretty_print)                  \
    do {                                                            \
        if (bl::logger::Logger::getLogger()->getLogLevel() <= pri) { \
            bl::logger::Logger::getLogger()->logToStream(pri, msg, size, pretty_print); \
        }                                                           \
    } while(0)


#define bl_log_debug(msg)  bl_log(bl::logger::Logger::eDebug, msg)
#define bl_log_info(msg)   bl_log(bl::logger::Logger::eInfo, msg)
#define bl_log_warn(msg)   bl_log(bl::logger::Logger::eWarning, msg)
#define bl_log_error(msg)  bl_log(bl::logger::Logger::eError, msg)
#define bl_log_fatal(msg)  bl_log(bl::logger::Logger::eFatal, msg)
#define bl_log_ascii(msg, size, pretty_print)  bl_log_ascii_pri(bl::logger::Logger::eDebug, msg, size, pretty_print)


// Prevent Visual Studio Compilers earlier than
// 2005 from attempting to build variable args
// which were not supported
#if defined(_MSC_VER) && _MSC_VER < 1400

#define LogSetup(program_name,log_dir,size_max,print_level,file_level,queue_level) bl::logger::Logger::setup(program_name,log_dir,size_max,print_level,file_level,queue_level);

#define Log(message) bl::logger::Logger::getLogger()->log(__FILE__, __FUNCTION__, message, __LINE__)

#define LogPri(priority,message) bl::logger::Logger::getLogger()->log(__FILE__, __FUNCTION__, priority, \
                                                                      message, __LINE__)
#else

#define LogSetup(...) bl::logger::Logger::setup(__VA_ARGS__);

#define Log(message,...) bl::logger::Logger::getLogger()->log(__FILE__, __FUNCTION__, message, __LINE__, \
                                                              ##__VA_ARGS__)

#define LogPri(priority,message,...) bl::logger::Logger::getLogger()->log(__FILE__, __FUNCTION__, priority, \
                                                                          message, __LINE__, ##__VA_ARGS__)

#endif //#if WIN32 && _MSC_VER < 1400


    } // ns: logger
} // ns: bl

#ifdef _MSC_VER
// See disable at the top for comment
#pragma warning(default: 4251)
#endif
#endif // __BL_LOG_HPP__
