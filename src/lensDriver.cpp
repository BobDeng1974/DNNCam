#include <iostream>
#include <boost/program_options.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <readline/readline.h>
#include <readline/history.h>

#include "motordriver.hpp"
#include "DNNCamServer.hpp"

using namespace std;
using namespace boost;
using namespace BoulderAI;

namespace po = boost::program_options;

static void lens_cout_log_handler(string output)
{
    cout << output << endl;
}

const char *convert_to_cstr(const std::string & s)
{
   return s.c_str(); 
}

#define DO_CMD(cmd)   \
        {if (false == cmd) { std::cout << "FAILED" << std::endl; } else { std::cout << "OK" << std::endl; } continue;}


void interactive(MotorDriverPtr m) {
    char c;
    int num_steps = 1;
    m.reset(new MotorDriver(true, lens_cout_log_handler));
    std::cout << "Verson 1.1.3: Type help for commands." << std::endl;
    po::options_description desc("Options");
    desc.add_options()
        ("help", "print help message")
        ("quit", "leave this program")
        ("init", "initialize motor")
        ("printpower", "prints the power registers")
        ("enablepowerlines", "enable the power lines")
        ("printzoom", "prints the zoom registers")
        ("zoomup", po::value<int>(), "zoom up number of steps")
        ("zoomdown", po::value<int>(), "zoom down number of steps")
        ("zoomhome", "homes zoom")
        ("zoomlocation", "prints zoom location")
        ("printfocus", "prints the focus registers")
        ("focusup", po::value<int>(), "focus up number of steps")
        ("focusdown", po::value<int>(), "focus down number of steps")
        ("focushome", "homes focus")
        ("focuslocation", "prints focus location")
        ("printiris", "prints the iris registers")
        ("irisup", po::value<int>(), "iris up number of steps")
        ("irisdown", po::value<int>(), "iris down number of steps")
        ("irishome", "homes iris")
        ("irislocation", "prints iris location")
        ("ircuton", "flips the ir-cut filter on")
        ("ircutoff", "flips the ir-cut filter off")
    ;
    std::istringstream iss;
    std::vector<std::string> args;
    std::string token;
    po::variables_map vm;
    string input;
    char *buf;

    rl_bind_key('\t', rl_abort);
    while((buf = readline("\n> ")) != NULL)
    {
        input = buf;
        if ("quit" == input) break;

        iss.clear();
        iss.str(input);
        args.clear();
        args.push_back("name");
        bool first = true;
        while (iss >> token) {
            // kind of ghetto, but add '--' to the first command, because we're just using boot::po
            // to do the parsing
            if(first)
            {
                token = "--" + token;
                first = false;
            }
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

        add_history(buf);

        if (vm.count("help")) {
            // remove the dashes since we're using boost::po to parse this interactively
            ostringstream remove_dashes_oss;
            remove_dashes_oss << desc;
            string remove_dashes = remove_dashes_oss.str();
            boost::replace_all(remove_dashes, "--", "");
            std::cout << remove_dashes << endl;
            continue;
        }
        if (vm.count("quit")) {
            break;
        }
        if (vm.count("init")) {
            DO_CMD(m->init());
        }
        if (vm.count("printpower")) {
            DO_CMD(m->printPower());
        }
        if (vm.count("enablepowerlines")) {
            DO_CMD(m->enablePowerLines());
        }
        if (vm.count("printzoom")) {
            DO_CMD(m->printZoom());
        }
        if (vm.count("zoomup")) {
            DO_CMD(m->zoomUp(vm["zoomup"].as<int>()));
        }
        if (vm.count("zoomdown")) {
            DO_CMD(m->zoomDown(vm["zoomdown"].as<int>()));
        }
        if (vm.count("zoomhome")) {
            DO_CMD(m->zoomHome());
        }
        if (vm.count("zoomlocation")) {
            auto ret = m->zoomAbsoluteLocation();
            std::cout << "Zoom absolute location: " << ret << std::endl;
            continue;
        }
        if (vm.count("printfocus")) {
            DO_CMD(m->printFocus());
        }
        if (vm.count("focusup")) {
            DO_CMD(m->focusUp(vm["focusup"].as<int>()));
        }
        if (vm.count("focusdown")) {
            DO_CMD(m->focusDown(vm["focusdown"].as<int>()));
        }
        if (vm.count("focushome")) {
            DO_CMD(m->focusHome());
        }
        if (vm.count("focuslocation")) {
            auto ret = m->focusAbsoluteLocation();
            std::cout << "Focus absolute location: " << ret << std::endl;
            continue;
        }
        if (vm.count("printiris")) {
            DO_CMD(m->printIris());
        }
        if (vm.count("irisup")) {
            DO_CMD(m->irisUp(vm["irisup"].as<int>()));
        }
        if (vm.count("irisdown")) {
            DO_CMD(m->irisDown(vm["irisdown"].as<int>()));
        }
        if (vm.count("irishome")) {
            DO_CMD(m->irisHome());
        }
        if (vm.count("irislocation")) {
            auto ret = m->irisAbsoluteLocation();
            std::cout << "Iris absolute location: " << ret << std::endl;
            continue;
        }
        if (vm.count("ircuton")) {
            DO_CMD(m->ircutOn());
        }
        if (vm.count("ircutoff")) {
            DO_CMD(m->ircutOff());
        }

    }
}

/*void rpcmode(DNNCamPtr dnncam)
{
    dnncam.reset(new DNNCam(true, cout_log_handler));
    DNNCamServerPtr server(new DNNCamServer(dnncam));
    server->run();
    }*/

int main(int argc, const char* argv[])
{
    MotorDriverPtr m;
    po::options_description desc{"Options"};
    desc.add_options()
        ("help,h", "Help screen");
    //("xmlrpc", "Run as xmlrpc client");
    try
    {
        po::variables_map vm;
        store(parse_command_line(argc, argv, desc), vm);
        notify(vm);

        if (vm.count("help")) {
            std::cout << desc << '\n';
        }/* else if (vm.count("xmlrpc")) {
            rpcmode(m);
            }*/ else {
            interactive(m);
        }
    } catch (const std::exception &e) {
        std::cout << "Error: " << e.what() << std::endl;
        std::cout << desc << std::endl;
    }
    return 0;
}
