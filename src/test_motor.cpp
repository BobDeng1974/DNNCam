#include <iostream>
#include <boost/program_options.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include "motordriver.hpp"

using namespace boost;
namespace po = boost::program_options;

const char *convert_to_cstr(const std::string & s)
{
   return s.c_str(); 
}

#define DO_CMD(cmd)   \
        {if (false == cmd) { std::cout << "FAILED" << std::endl; } else { std::cout << "OK" << std::endl; }; continue;}


int main()
{
    char c;
    MotorDriver m;
    std::cout << "Type --help for commands. Note that the -- (two dashes) are required for all the commands. Type quit or --quit to exit." << std::endl;
    po::options_description desc("Options");
    desc.add_options()
        ("help", "print help message")
        ("quit", "leave this program")
        ("init", "initialize motor")
        ("enablepowerlines", "enable the power lines")
        ("enablezoom", "enables the zoom")
        ("disablezoom", "disables the zoom")
        ("zoomin", po::value<int>(), "zoom in number of steps")
        ("zoomout", po::value<int>(), "zoom out number of steps")
        ("enablefocus", "enables the focus")
        ("disablefocus", "disables the focus")
        ("focusin", po::value<int>(), "focus in number of steps")
        ("focusout", po::value<int>(), "focus out number of steps")
        ("enableiris", "enables the iris")
        ("disableiris", "disables the iris")
        ("irisin", po::value<int>(), "iris in number of steps")
        ("irisout", po::value<int>(), "iris out number of steps")
    ;
    std::string input;
    std::istringstream iss;
    std::vector<std::string> args;
    std::string token;
    po::variables_map vm;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        if ("quit" == input) break;

        iss.clear();
        iss.str(input);
        args.clear();
        args.push_back("name");
        while (iss >> token) {
            args.push_back(token);
        }
        auto beg = boost::make_transform_iterator(args.begin(), 
                convert_to_cstr);
        auto end = boost::make_transform_iterator(args.end(), 
                convert_to_cstr);

        const std::vector<const char*> vc { beg, end };
        vm.clear();
        try {
        po::store(po::parse_command_line(vc.size(), vc.data(), desc), vm);
        po::notify(vm);    
        } catch (std::exception &e) {
            std::cout << "Error: " << e.what() << std::endl;
            std::cout << desc << std::endl;
        }

        if (vm.count("help")) {
            std::cout << desc << "\n";
            continue;
        }
        if (vm.count("quit")) {
            break;
        }
        if (vm.count("init")) {
            DO_CMD(m.init());
        }
        if (vm.count("enablepowerlines")) {
            DO_CMD(m.enablePowerLines());
        }
        if (vm.count("enablezoom")) {
            DO_CMD(m.enableZoom());
        }
        if (vm.count("disablezoom")) {
            DO_CMD(m.disableZoom());
        }
        if (vm.count("zoomin")) {
            DO_CMD(m.zoomIn(vm["zoomin"].as<int>()));
        }
        if (vm.count("zoomout")) {
            DO_CMD(m.zoomOut(vm["zoomout"].as<int>()));
        }
        if (vm.count("enablefocus")) {
            DO_CMD(m.enableFocus());
        }
        if (vm.count("disablefocus")) {
            DO_CMD(m.disableFocus());
        }
        if (vm.count("focusin")) {
            DO_CMD(m.focusIn(vm["focusin"].as<int>()));
        }
        if (vm.count("focusout")) {
            DO_CMD(m.focusOut(vm["focusout"].as<int>()));
        }
        if (vm.count("enableiris")) {
            DO_CMD(m.enableIris());
        }
        if (vm.count("disableiris")) {
            DO_CMD(m.disableIris());
        }
        if (vm.count("irisin")) {
            DO_CMD(m.irisIn(vm["irisin"].as<int>()));
        }
        if (vm.count("irisout")) {
            DO_CMD(m.irisOut(vm["irisout"].as<int>()));
        }
    }
    return 0;
}
