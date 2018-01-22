#include <stdexcept>
#include <iostream>
#include <sstream>

#ifndef WIN32
#include <unistd.h>  // daemon
#endif

#include <exception>
#include <boost/program_options.hpp>

#include "DNNCam.hpp"

#include "frame_processor.hpp"

using namespace std;
using namespace BoulderAI;

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

    //W=720;
    //H=720;
    
    // full hd
    W=1920;
    H=1080;

    // camera full res
    //W = 3840;
    //H = 2160;

    int ret = parse_arguments(argc, argv);
    if (ret != 0)
    {
        return ret;
    }

    DNNCamPtr camera;
    std::cout << "Initializing camera. Frame size: " << W << "x" << H << std::endl;
    
    camera.reset(new DNNCam(0, 0, W, H, W, H));
    camera->init();
        
    FrameProcessorPtr frame_proc;
    frame_proc.reset(new FrameProcessor(W, H));
    frame_proc->start_workers();
    
    static struct timeval last_tv;
    gettimeofday(&last_tv, NULL);

    /*Argus::Range < uint64_t > exp_range(16000, 16000);
    camera->set_exposure_time(exp_range);
    Argus::Range < float > gain_range(1, 1);
    camera->set_gain(gain_range);
    camera->set_frame_duration(exp_range);*/
    //camera->set_auto_exposure(true);
    
    while(running)
    {
        static bool auto_exp = true;
	struct timeval now_tv, result_tv;

	gettimeofday(&now_tv, NULL);
	timersub(&now_tv, &last_tv, &result_tv);
	last_tv = now_tv;

	cout << "Now: " << now_tv.tv_sec << "." << setfill('0') << setw(6) << now_tv.tv_usec << " Diff: " << result_tv.tv_sec << "." << setfill('0') << setw(6) << result_tv.tv_usec << endl;
	
        FrameCollection col;
        bool was_frame_dropped;
        uint64_t frame_num;
        col.frame_rgb = camera->grab(was_frame_dropped, frame_num);  // This is a blocking call. Grab must be called before any grab_*
        //col.frame_y = camera->grab_y();
        //col.frame_u = camera->grab_u();
        //col.frame_v = camera->grab_v();

        /*if(now_tv.tv_sec - last_tv.tv_sec > 5)
        {
            auto_exp = !auto_exp;
            camera->set_auto_exposure(auto_exp);
            
            if(!auto_exp)
            {
                Argus::Range < uint64_t > exp_range(2000, 2000);
                camera->set_exposure_time(exp_range);
                Argus::Range < float > gain_range(1, 1);
                camera->set_gain(gain_range);
                //_camera->set_frame_duration(30000);
            }
            else
            {
                Argus::Range < float > gain_range(5, 5);
                camera->set_gain(gain_range);
                //_camera->set_frame_duration(33333333);
            }
            
            camera->get_exposure_time();
            camera->get_gain();
            cout << "Auto exposure is " << auto_exp << endl;
	    }*/

	
        
        //frame_proc->process_frame(col);
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
