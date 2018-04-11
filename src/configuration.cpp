
#include <fstream>
#include <iostream>

#include "configuration.hpp"

using namespace std;

namespace BoulderAI
{

std::string Configuration::_models_directory;
bool Configuration::_initialized;

int Configuration::_zoom_start;
int Configuration::_zoom_up_direction;
int Configuration::_zoom_home_direction;
int Configuration::_zoom_home_max_steps;
int Configuration::_zoom_home_step_size;
bool Configuration::_zoom_has_limit;

int Configuration::_focus_start;
int Configuration::_focus_up_direction;
int Configuration::_focus_home_direction;
int Configuration::_focus_home_max_steps;
int Configuration::_focus_home_step_size;
bool Configuration::_focus_has_limit;

int Configuration::_iris_start;
int Configuration::_iris_up_direction;
int Configuration::_iris_home_direction;
int Configuration::_iris_home_max_steps;
int Configuration::_iris_home_step_size;
bool Configuration::_iris_has_limit;

static void cout_log_handler(std::string output)
{
    cout << output << endl;
}
    
boost::function < void(std::string) > Configuration::_log_callback = cout_log_handler;
    
po::variables_map Configuration::_vm;
po::options_description Configuration::_options("Configuration");

std::string Configuration::_config_filename="/etc/lensdriver.cfg";

void Configuration::set_log_handler(boost::function < void(std::string) > handler)
{
    _log_callback = handler;
}

void Configuration::_load_config_file(void)
{
    _options.add_options()
	("models_directory", po::value<std::string>(&_models_directory)->default_value("/usr/bin/wrModels/"), "Wrnch Models Directory")
        ("zoom_start", po::value<int>(&_zoom_start)->default_value(0), "absolute zoom start offset")
        ("zoom_up_direction", po::value<int>(&_zoom_up_direction)->default_value(0), "0 or 1 for direction")
        ("zoom_home_direction", po::value<int>(&_zoom_home_direction)->default_value(0), "0 or 1 for direction")
        ("zoom_home_max_steps", po::value<int>(&_zoom_home_max_steps)->default_value(100), "maximum steps to take while homing")
        ("zoom_home_step_size", po::value<int>(&_zoom_home_step_size)->default_value(5), "number of steps to take while homing")
        ("zoom_has_limit", po::value<bool>(&_zoom_has_limit)->default_value(true), "zoom has limit")
        ("focus_start", po::value<int>(&_focus_start)->default_value(0), "absolute focus start offset")
        ("focus_up_direction", po::value<int>(&_focus_up_direction)->default_value(0), "0 or 1 for direction")
        ("focus_home_direction", po::value<int>(&_focus_home_direction)->default_value(0), "0 or 1 for direction")
        ("focus_home_max_steps", po::value<int>(&_focus_home_max_steps)->default_value(100), "maximum steps to take while homing")
        ("focus_home_step_size", po::value<int>(&_focus_home_step_size)->default_value(5), "number of steps to take while homing")
        ("focus_has_limit", po::value<bool>(&_focus_has_limit)->default_value(true), "focus has limit")
        ("iris_start", po::value<int>(&_iris_start)->default_value(0), "absolute iris start offset")
        ("iris_up_direction", po::value<int>(&_iris_up_direction)->default_value(0), "0 or 1 for direction")
        ("iris_home_direction", po::value<int>(&_iris_home_direction)->default_value(0), "0 or 1 for direction")
        ("iris_home_max_steps", po::value<int>(&_iris_home_max_steps)->default_value(1000), "maximum steps to take while homing")
        ("iris_home_step_size", po::value<int>(&_iris_home_step_size)->default_value(5), "number of steps to take while homing")
        ("iris_has_limit", po::value<bool>(&_iris_has_limit)->default_value(false), "iris has limit")
    ;
    std::ifstream ifs;
    ifs.open(_config_filename);
    if (ifs.fail())
    {
        ostringstream oss;
        oss << "Unable to open config file: " << _config_filename;
        _log_callback(oss.str());
        ifs.open("./lensdriver.cfg");
        if (ifs.fail())
        {
            _log_callback("Unable to open ./lensdriver.cfg either.");
        }
        else
        {
            _log_callback("Opened ./lensdriver.cfg.");
        }
    }

    try
    {
         po::store(po::parse_config_file(ifs, _options), _vm);
         po::notify(_vm);
    }
    catch (po::error &e)
    {
        ostringstream oss;
        oss << "Error parsing config file: " << e.what();
        _log_callback(oss.str());
        exit(1);
    }

    _log_callback("Loaded config file.");
    _initialized = true;
}


} // namespace BoulderAI
