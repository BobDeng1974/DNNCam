#pragma once

#include <boost/program_options.hpp>

namespace po=boost::program_options;

namespace BoulderAI
{

class Configuration
{
public:
    static void set_log_handler(boost::function < void(std::string) > handler);
    
    static int zoom_start() { init(); return _zoom_start; }
    static int zoom_up_direction() { init(); return _zoom_up_direction; }
    static int zoom_home_direction() { init(); return _zoom_home_direction; }
    static int zoom_home_max_steps() { init(); return _zoom_home_max_steps; }
    static int zoom_home_step_size() { init(); return _zoom_home_step_size; }
    static bool zoom_has_limit() { init(); return _zoom_has_limit; }

    static int focus_start() { init(); return _focus_start; }
    static int focus_up_direction() { init(); return _focus_up_direction; }
    static int focus_home_direction() { init(); return _focus_home_direction; }
    static int focus_home_max_steps() { init(); return _focus_home_max_steps; }
    static int focus_home_step_size() { init(); return _focus_home_step_size; }
    static bool focus_has_limit() { init(); return _focus_has_limit; }

    static int iris_start() { init(); return _iris_start; }
    static int iris_up_direction() { init(); return _iris_up_direction; }
    static int iris_home_direction() { init(); return _iris_home_direction; }
    static int iris_home_max_steps() { init(); return _iris_home_max_steps; }
    static int iris_home_step_size() { init(); return _iris_home_step_size; }
    static bool iris_has_limit() { init(); return _iris_has_limit; }

protected:
    static void _load_config_file();
    static void init()
    {
        if (!_initialized)
        {
            _load_config_file();
        }
    }

    static std::string _config_filename;
    static bool _initialized;

    static po::options_description _options;
    static po::variables_map _vm;

    // Zoom configuration
    static int _zoom_start;
    static int _zoom_up_direction;
    static int _zoom_home_direction;
    static int _zoom_home_max_steps;
    static int _zoom_home_step_size;
    static bool _zoom_has_limit;

    // Focus configuration
    static int _focus_start;
    static int _focus_up_direction;
    static int _focus_home_direction;
    static int _focus_home_max_steps;
    static int _focus_home_step_size;
    static bool _focus_has_limit;

    // Iris configuration
    static int _iris_start;
    static int _iris_up_direction;
    static int _iris_home_direction;
    static int _iris_home_max_steps;
    static int _iris_home_step_size;
    static bool _iris_has_limit;

    static boost::function < void(std::string) > _log_callback;
};

} // namepsace BoulderAI
