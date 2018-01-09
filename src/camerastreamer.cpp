#include <stdexcept>
#include <iostream>
#include <sstream>

#ifndef WIN32
#include <unistd.h>  // daemon
#endif

#include <exception>
#include <boost/program_options.hpp>

//#include "imx377camera.hpp"
#include "ArgusCamera.hpp"

#include "frame_processor.hpp"
#include "log.hpp"

using namespace std;

namespace po = boost::program_options ;
static bool verbose=false;
static int W = -1;
static int H = -1;

static bool running = true;

void signalHandler(int signum) {
    std::cout << "caught signal " << signum << std::endl;
    running = false;
}

static int parse_arguments(int argc, char *argv[])
{
    bool args_valid = true; 

    /* Set up program options for processing */
    po::options_description visible_options("Usage: camerastreamer [options] \n\nOptions are: ");
    visible_options.add_options()
        ("help", "Print this message.")
        ("verbose,v", "Print more information." )
        ("width,w", po::value < int >(), "Frame width")
        ("height,h", po::value < int >(), "Frame height")
        ;
    
    /* Process them */
    try {
        po::variables_map vm;        
        po::store(po::command_line_parser(argc, argv).options(visible_options).run(), vm);
        po::notify(vm);    

        if (vm.count("help")) 
        {
            std::cerr << visible_options << std::endl;
            return 1;
        }

        if (vm.count("verbose"))
        {
            verbose = true; 
        }

        if(vm.count("width"))
        {
            W = vm["width"].as < int >();
        }

        if(vm.count("height"))
        {
            H = vm["height"].as < int >();
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

    // full hd
    //W=1920;
    //H=1080;

    // camera full res
    W = 3864;
    H = 2196;

    int ret = parse_arguments(argc, argv);
    if (ret != 0)
    {
        return ret;
    }

    ArgusCameraPtr camera;
    std::cout << "Initializing camera. Frame size: " << W << "x" << H << std::endl;
    
    //camera.reset(new ArgusCamera(ctx, 1944, 1116, W, H));
    camera.reset(new ArgusCamera(0, 0, 1920, 1080, 1920, 1080));
    camera->init();
        
    FrameProcessorPtr frame_proc;
    frame_proc.reset(new FrameProcessor(1920, 1080));
    frame_proc->start_workers();
    
    while(1)
    {
        static bool auto_exp = true;
        static time_t last = time(NULL);
        
        FrameCollection col;
        col.frame_rgb = camera->grab();  // This is a blocking call. Grab must be called before any grab_*
        col.frame_y = camera->grab_y();
        col.frame_u = camera->grab_u();
        col.frame_v = camera->grab_v();

        time_t now = time(NULL);
        if(now - last > 5)
        {
            auto_exp = !auto_exp;
            camera->set_auto_exposure(auto_exp);
            
            if(!auto_exp)
            {
                camera->set_exposure_time(2000000);
                camera->set_gain(10);
                //_camera->set_frame_duration(30000);
            }
            else
            {
                camera->set_gain(50);
                //_camera->set_frame_duration(33333333);
            }
            
            camera->get_exposure_time();
            camera->get_gain();
            bl_log_info("Auto exposure is " << auto_exp);
            last = now;
        }
        
        frame_proc->process_frame(col);
    }
    
    frame_proc->wait_for_queued_images();
    
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
