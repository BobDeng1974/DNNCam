#include <stdexcept>
#include <iostream>
#include <sstream>
#include <fstream>

#ifndef WIN32
#include <unistd.h>  // daemon
#endif

#include <exception>
#include <boost/program_options.hpp>

#include "DNNCam.hpp"
#include "DNNCamServer.hpp"

#include "frame_processor.hpp"

using namespace std;
using namespace BoulderAI;

namespace po = boost::program_options ;

static bool running = true;

void signalHandler(int signum)
{
    std::cout << "caught signal " << signum << std::endl;
    running = false;
}

static int parse_arguments(int argc, char *argv[])
{
    bool args_valid = true; 

    /* Set up program options for processing */
    po::options_description visible_options("Usage: camerastreamer [options] \n\nOptions are: ");
    visible_options.add_options()
        ("help,h", "Print this message.")
        ;

    visible_options.add(DNNCam::GetOptions());
    
    /* Process them */
    try {
        string filename = "/etc/dnncam.conf";
        std::ifstream ifs;
        ifs.open(filename);
        if(ifs.fail())
        {
            ostringstream oss;
            oss << "Unable to open " << filename;
            cerr << oss.str() << endl;
        }
        
        po::variables_map vm;        
        po::store(po::command_line_parser(argc, argv).options(visible_options).run(), vm);
        po::store(po::parse_config_file(ifs, DNNCam::GetOptions()), vm);
        po::notify(vm);    

        if (vm.count("help")) 
        {
            std::cerr << visible_options << std::endl;
            return 1;
        }
    }
    catch(std::exception& e) {
        std::cerr << "Error " << e.what() << std::endl;
        args_valid = false; 
    }
    catch(...) {
        std::cerr << "Exception of unknown type!\n";
        return -2; 
    }
    
    /* Make sure that they were valid */
    if (!args_valid)
    {
        std::cout << std::endl << visible_options << std::endl;
        return 2; 
    }

    return 0;
}

int run(int argc, char** argv)
{
    srand(time(NULL));

    DNNCamPtr camera;
    
    camera.reset(new DNNCam());

    int ret = parse_arguments(argc, argv);
    if (ret != 0)
    {
        return ret;
    }
    
    camera->init();
    
    std::cout << "Initialized camera. Frame size: " << camera->get_output_width() << "x" << camera->get_output_height() << std::endl;

    DNNCamServerPtr server(new DNNCamServer(camera));
    boost::thread *server_thread(new boost::thread(boost::bind(&DNNCamServer::run, server)));
        
    FrameProcessorPtr frame_proc;
    frame_proc.reset(new FrameProcessor(camera->get_output_width(), camera->get_output_height()));
    frame_proc->start_workers();
    
    while(running)
    {
        static bool auto_exp = true;
        static time_t last = time(NULL);
        
        FrameCollection col;
        bool was_frame_dropped;
        col.frame_rgb = camera->grab(was_frame_dropped);  // This is a blocking call. Grab must be called before any grab_*
        col.frame_y = camera->grab_y();
        col.frame_u = camera->grab_u();
        col.frame_v = camera->grab_v();

        frame_proc->process_frame(col);
    }

    frame_proc->wait_for_queued_images();
    
    server->stop();
    delete server_thread;
    
    return ret; 
}
        
int main(int argc, char** argv)
{
    int ret = -1;
    signal(SIGINT, signalHandler);
    try
    {
        ret = run(argc, argv);
    }
    catch (const std::exception &e)
    {
        std::cout << "Unhandled exception: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "Caught an unknown type of unhandled exception." << std::endl;
    }
    return ret;
}
