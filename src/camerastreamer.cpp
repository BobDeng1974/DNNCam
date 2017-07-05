#include <stdexcept>
#include <iostream>
#include <sstream>

#ifndef WIN32
#include <unistd.h>  // daemon
#endif

#include <exception>
#include <boost/program_options.hpp>

#include "imx377camera.hpp"

#include "datasource_camera.hpp"
#include "frame_processor.hpp"

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

    W=1920;
    H=1080;

    int ret = parse_arguments(argc, argv);
    if (ret != 0)
    {
        return ret;
    }

    FrameProcessorPtr frame_proc;
    frame_proc.reset(new FrameProcessor(W, H));

    DataSourceBasePtr data_source;
    CameraPtr camera;
    auto ctx = std::make_shared<camera_context>();
    ctx->width = W;
    ctx->height = H;
    ctx->frame_processor = frame_proc;
    std::cout << "Initializing camera. Frame size: " << W << "x" << H << std::endl;
    camera.reset(new Imx377Camera(ctx));
    camera->init();
    camera->start_capture();
        
    frame_proc->start_workers();
    
    if (camera) {
        camera->start_capture();
    }
    if (false == data_source->usesCallback()) {
        data_source->Process();
    } else {
        while (running) {
            sleep(1);
        }
    }
    if (camera) {
        camera->stop();
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
