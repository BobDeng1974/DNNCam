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
    int num_steps = 1;
    MotorDriver m;
    std::cout << "Type --help for commands. Note that the -- (two dashes) are required for all the commands. Type quit or --quit to exit." << std::endl;
    po::options_description desc("Options");
    desc.add_options()
        ("help", "print help message")
        ("quit", "leave this program")
        ("init", "initialize motor")
        ("printpower", "prints the power registers")
        ("enablepowerlines", "enable the power lines")
        ("printzoom", "prints the zoom registers")
        ("zoomin", po::value<int>(), "zoom in number of steps")
        ("zoomout", po::value<int>(), "zoom out number of steps")
        ("zoomdir", po::value<int>(), "zoom to direction(0 sets ZOOM_DIR to low, else to high), takes --steps arg")
        ("zoomhome", po::value<int>(), "tries to home to direction, takes --steps arg as max number of steps it will try")
        ("steps", po::value<int>(&num_steps), "number of steps")
        ("printfocus", "prints the focus registers")
        ("focusin", po::value<int>(), "focus in number of steps")
        ("focusout", po::value<int>(), "focus out number of steps")
        ("focusdir", po::value<int>(), "focus to direction(0 sets FOCUS_DIR to low, else to high), takes --steps arg")
        ("focushome", po::value<int>(), "tries to home to direction, takes --steps arg as max number of steps it will try")
        ("printiris", "prints the iris registers")
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
        if (vm.count("printpower")) {
            DO_CMD(m.printPower());
        }
        if (vm.count("enablepowerlines")) {
            DO_CMD(m.enablePowerLines());
        }
        if (vm.count("printzoom")) {
            DO_CMD(m.printZoom());
        }
        if (vm.count("zoomin")) {
            DO_CMD(m.zoomIn(vm["zoomin"].as<int>()));
        }
        if (vm.count("zoomout")) {
            DO_CMD(m.zoomOut(vm["zoomout"].as<int>()));
        }
        if (vm.count("zoomdir")) {
            DO_CMD(m.zoomDir(vm["zoomdir"].as<int>(), num_steps));
        }
        if (vm.count("zoomhome")) {
            DO_CMD(m.zoomHome(vm["zoomhome"].as<int>(), num_steps));
        }
        if (vm.count("printfocus")) {
            DO_CMD(m.printFocus());
        }
        if (vm.count("focusin")) {
            DO_CMD(m.focusIn(vm["focusin"].as<int>()));
        }
        if (vm.count("focusout")) {
            DO_CMD(m.focusOut(vm["focusout"].as<int>()));
        }
        if (vm.count("focusdir")) {
            DO_CMD(m.focusDir(vm["focusdir"].as<int>(), num_steps));
        }
        if (vm.count("focushome")) {
            DO_CMD(m.focusHome(vm["focushome"].as<int>(), num_steps));
        }
        if (vm.count("printiris")) {
            DO_CMD(m.printIris());
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
