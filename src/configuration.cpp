#include "configuration.hpp"

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

po::variables_map Configuration::_vm;
po::options_description Configuration::_options("Configuration");

std::string Configuration::_config_filename="/etc/lensdriver.cfg";

void Configuration::_load_config_file(void)
{
    _options.add_options()
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
        bl_log_warn("Unable to open: " << _config_filename);
        ifs.open("./lensdriver.cfg");
        if (ifs.fail())
        {
            bl_log_warn("Unable to open ./lensdriver.cfg either.");
        }
        else
        {
            bl_log_info("Opened ./lensdriver.cfg.");
        }
    }

    try
    {
         po::store(po::parse_config_file(ifs, _options), _vm);
         po::notify(_vm);
    }
    catch (po::error &e)
    {
        bl_log_warn("Error parsing config file: " << e.what());
        exit(1);
    }

    bl_log_info("Loaded config file.");
    _initialized = true;
}

