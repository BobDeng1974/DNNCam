
#ifdef USE_BOOST_FILESYSTEM_3
#define BOOST_FILESYSTEM_VERSION 3
#else
#define BOOST_FILESYSTEM_VERSION 2
#endif

#include "include/log.hpp"

#include <boost/filesystem/operations.hpp>

#pragma warning (disable : 4245)
#include <boost/filesystem/convenience.hpp>
#pragma warning (default : 4245)

#include <boost/filesystem/path.hpp>
#include <cstdio>


namespace bl
{
    namespace logger
    {
        /* Outside because they need to be for static initialization */
        Logger *Logger::instance = NULL;
        boost::recursive_mutex Logger::mux;

        std::string Logger::progname = "logger";
        size_t Logger::size_max = static_cast<size_t>(1E6);
        const size_t Logger::default_size_max = static_cast<const size_t>(1E6);

        Logger::LogLevel Logger::print_level = eInfo;
        Logger::LogLevel Logger::file_level = eDebug;
        Logger::LogLevel Logger::queue_level = eWarning;

        std::string Logger::logdir;
        std::string Logger::header;

        // Changes has to be protected by mutex!
        int Logger::max_log_file_count = 10;

        bool Logger::quiet_mode = false;

    void Logger::setup(std::string program_name,
               std::string log_dir,
               int size_max,
               LogLevel print_level,
               LogLevel file_level,
               LogLevel queue_level,
               bool quiet_mode,
               std::string header)
        {
            boost::recursive_mutex::scoped_lock lock(Logger::mux);
            if (instance == NULL)
            {
                Logger::progname = program_name;
                fileSettings(size_max, Logger::max_log_file_count);
                Logger::print_level = print_level;
                Logger::file_level = file_level;
                Logger::logdir = log_dir;
                Logger::queue_level = queue_level;
                Logger::quiet_mode = quiet_mode;
                Logger::header = header;
            }
            else
            {
                LogPri(eWarning, "Attempt to call Logger::setup after initialization ignored.");
            }
        }

    Logger::LogLevel Logger::getLogLevel() const
    {
        return print_level < file_level ? print_level : (file_level < queue_level ? file_level : queue_level);
    }

    void Logger::fileSettings(const int &size_max,const int &max_files) {
        boost::recursive_mutex::scoped_lock lock(Logger::mux);
        if ((size_max < 1E3) || (size_max > 1E9)) {
            Logger::size_max = static_cast<size_t>(Logger::default_size_max);
            std::ostringstream ss;
            ss << "Warning: Max file size of " << size_max << " is disallowed. Must be in range 1E3 <= size <= 1E9. Defaulting to " << Logger::size_max << ".";
            LogPri(eWarning, ss.str());
        } else {
            Logger::size_max = size_max;
        }
        setMaxLogFileCount(max_files);
    }

        Logger *Logger::getLogger(void)
        {
            if (instance == NULL)
            {
                boost::recursive_mutex::scoped_lock lock(Logger::mux);
                if (instance == NULL)
                {
                    instance = new Logger();
                    atexit(Logger::killLogger);
                }
            }

            return instance;
        }

        void Logger::setMaxLogFileCount(const int pi_maxLogCount)
        {
            boost::recursive_mutex::scoped_lock lock(Logger::mux);
            if (pi_maxLogCount < 0)
            {
                std::ostringstream msg;
                msg << "BL::Logger: Invalid maxLogCount value: "
                    << pi_maxLogCount;
                throw std::runtime_error(msg.str());
            }
            max_log_file_count = pi_maxLogCount;
        }


        void Logger::killLogger(void)
        {
            boost::recursive_mutex::scoped_lock lock(Logger::mux);
            if (instance != NULL)
            {
                delete instance;
                instance = NULL;
            }
        }

        void Logger::log(const std::string &file, const std::string &func, int line, std::string message)
        {
            boost::recursive_mutex::scoped_lock lock(Logger::mux);
            this->getStream(file, func, line) << message;
        }

        void Logger::log(const char *file, const char *func, std::string message, int line, ...)
        {
            boost::recursive_mutex::scoped_lock lock(Logger::mux);
            va_list ap;
            char buf[4096];

            va_start(ap, line);
            vsnprintf(buf, sizeof(buf), message.c_str(), ap);
            message = std::string(buf);
            va_end(ap);

            this->getStream(file, func, line) << message;
        }

        void Logger::log(const char *file, const char *func, LogLevel priority, std::string message,
                         int line, ...)
        {
            boost::recursive_mutex::scoped_lock lock(Logger::mux);

            va_list ap;
            char buf[4096];

            va_start(ap, line);
            vsnprintf(buf, sizeof(buf), message.c_str(), ap);
            message = std::string(buf);
            va_end(ap);

            this->getStream(file, func, line, priority) << message;
        }

        void Logger::enqueue(LogLevel priority, const std::string &message)
        {
            if (priority >= queue_level)
            {
                Message m;
                m.priority = priority;
                m.message = message;
                msgQueue.push(m);
                if (msgQueue.size() > MAX_QUEUE_SIZE)
                    msgQueue.pop();
            }
        }


        DualStream Logger::getStream(const std::string &file, const std::string &func, int line)
        {
            return DualStream(*this, file, func, line, Logger::eInfo);
        }

        DualStream Logger::getStream(const char *file, const char *func, int line)
        {
            return DualStream(*this, file, func, line, Logger::eInfo);
        }

        DualStream Logger::getStream(const char *file, const char *func, int line, LogLevel priority)
        {
            return DualStream(*this, file, func, line, priority);
        }

        DualStream Logger::getStream(const char *file, const char *func, int line, LogLevel priority, const char *colorStr)
        {
            return DualStream(*this, file, func, line, priority, colorStr);
        }

        void Logger::logToStream(LogLevel priority, uint8_t *msg, const int msgSize,
                                 const bool pretty = false)
        {
            std::string asciiString, paddedString;
            paddedString = "";
            char c;
            char tmp[10];
            int lineLength = 16;

            for (int i = 0; i < msgSize; i++)
            {
                //Get char representation of byte
                c = msg[i];

                //add char to padded string, spacing lines up with following ascii char code
                if (pretty)
                {
                    if (!isprint((int)c))
                        c = ' ';

                    sprintf(tmp, " %c  ", c);
                }
                else
                {  //add char and ascii in one go
                    if (isprint(c))
                        sprintf(tmp, "%c", c);
                    else if ('\n' == c)
                        sprintf(tmp, "\\n");
                    else
                        sprintf(tmp, "\\%02x", 0xFF & c);
                }

                paddedString += tmp;
                if (pretty) // haven't added ascii yet
                {
                    //add 3-digit ascii code (plus space) to separate string
                    sprintf(tmp, "\\%02x ", msg[i]);
                    asciiString += tmp;
                    //if we've hit the lineLength, time to break the line
                    if (((i + 1) % lineLength) == 0)
                    {
                        //break line,
                        //add string of ascii codes to next line,
                        //break line again for next string of chars
                        paddedString += "\n" + asciiString + "\n";
                        asciiString = "";
                    }
                }
            }

            //if there's still ascii codes left, add them to the string of chars
            if (pretty && asciiString.length() > 0)
                paddedString += "\n" + asciiString + "\n";

            std::ostringstream logMessage;

            if (pretty)
                logMessage << "\n";

            //logMessage << msgSize << "b:";

            if (pretty)
                logMessage << "\n";

            LogStreamPri(priority) << logMessage.str() << paddedString;
        }

        std::string Logger::convertNonPrintables(std::string input)
        {
            std::string output;

            for(size_t i = 0; i < input.length(); i++)
            {
                char c = input[i];
                if(isprint(c))
                {
                    output.push_back(c);
                }
                else
                {
                    char converted[4];
                    snprintf(converted, sizeof(converted), "\\%02x", c);
                    output.append(converted);
                }
            }

            return output;
        }

        std::string Logger::getLogFilename(int num)
        {
            boost::filesystem::path p;

            /* First see if the directory exists. If not, try to create it */
            if (("" != logdir) && (!boost::filesystem::exists(logdir)))
            {
                /* Note that this used to be create_directory; which was not recursive */
                try
                {   
                    boost::filesystem::create_directories(logdir);
                }
                catch (...)
                {
                    std::cerr << "Could not create log directory: " << logdir << std::endl;
                }
            }

            /* Now check to see if we have a good directory */
            if ("" == logdir || ! boost::filesystem::exists(logdir) || !boost::filesystem::is_directory(logdir))
            {
                /* Print a warning if the directory was set incorrectly */
                if ("" != logdir)
                    std::cerr << "The log directory is either not a directory or couldn't be created. Using default." << std::endl;

                p = boost::filesystem::current_path();
                boost::filesystem::create_directory("logout");
                p = p / "logout";
            }
            else
            {
                p = this->logdir;
            }

            if (num == 0)
            {
                p = p / (progname + ".log");
            }
            else
            {
                std::ostringstream nameAndNum;
                nameAndNum << progname << "." << num << ".log";
                p = p / (nameAndNum.str());
            }

            #ifdef USE_BOOST_FILESYSTEM_3
            return p.string();
            #else
            return p.native_file_string();
            #endif
        }

        void Logger::rotate()
        {
            boost::recursive_mutex::scoped_lock lock(Logger::mux);
            
            if(file_level == eNone)
                return;
            
            output.close();

            /* Bump existing files, dropping the last off at the end */
            for(int idx = max_log_file_count-2; idx >= 0; idx--)
            {
                if (boost::filesystem::exists(getLogFilename(idx)))
                {
                    try
                    {
                        boost::filesystem::remove(getLogFilename(idx + 1));
                        boost::filesystem::rename(getLogFilename(idx), getLogFilename(idx + 1));
                    }
                    catch (const boost::filesystem::filesystem_error &)
                    {
                        // We can't log this, but it will do the rotation the next time around
                        // if the error leading to the exception has been resolved, and violating
                        // the log size is better than crashing the application.
                    }
                }
            }

            openLog();
        }

        void Logger::openLog()
        {
            boost::recursive_mutex::scoped_lock lock(Logger::mux);

            if(file_level == eNone)
                return;
            
            std::string filename = getLogFilename(0);
            output.open(filename.c_str(), std::ios::app);
            output << header; 
        }

        void DualStream::flush(void)
        {
            boost::recursive_mutex::scoped_lock lock(Logger::mux);
            if(str)
            {
                if(this->priority >= logger.print_level)
                {
                    #ifdef _MSC_VER
                    std::cout << str->str() << std::endl;
                    #else
                    std::cout << this->colorStr << str->str() << "\e[0m" << std::endl;
                    #endif
                }

                const char *level = NULL;
                switch(priority)
                {
                case Logger::eDebug:
                    level = "DEBUG";
                    break;
                case Logger::eInfo:
                    level = "INFO";
                    break;
                case Logger::eWarning:
                    level = "WARN";
                    break;
                case Logger::eError:
                    level = "ERROR";
                    break;
                case Logger::eFatal:
                    level = "FATAL";
                    break;
                case Logger::eNone:
                    level = "NONE";
                    break;  // should not get here...
                }

                if(this->priority >= logger.file_level)
                {
                    if(static_cast<size_t>(logger.output.tellp()) >= logger.size_max)
                        logger.rotate();


                    if (! logger.quiet_mode) {
                        logger.output <<  boost::posix_time::to_iso_extended_string(m_ts) << ':' << level
                            << ':' << file << '[' << line << "]:" << func << "():" << str->str()
                            << std::endl;
                    } else {
                        logger.output <<  boost::posix_time::to_iso_extended_string(m_ts) << ':' << level
                            << ' ' << str->str() << std::endl;
                    }
                    logger.output.flush();
                }

                logger.enqueue(priority, str->str());

                delete str;
                str = NULL;
            }
        }

    } // ns: logger
} // ns: bl
