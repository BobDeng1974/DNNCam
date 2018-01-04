#include <iostream>
#include <boost/program_options.hpp>

#include <include/ArgusCamera.hpp>
#include <include/log.hpp>

using namespace boost;
namespace po = boost::program_options;
int run (ArgusCameraPtr a, po::variables_map* vm)
{
	bool ret = true;
	std::vector<float> wb = {1.0,1.0,1.0,1.0};
	//device_id, Width, Height, Min Exposure, Max exposure, Min Gain, Max Gain, AWB mode, AWB Gains, fps, timeout, exposure comp
//	a.reset(new ArgusCamera(0,320,240,0.003,0.033,1.0,16.0,AutoWhiteBalanceMode::AUTO, wb,29.99,1.0,0.0));
	a.reset(new ArgusCamera(vm));

	ret = a->init();
	//TODO: This changes the camera resolution but the argus output resolution is configured in init().
	//NvBuffer is allocated during requestFrame(). 
	//Hence the output is resized from argus output resolution to NvBuffer resolution
	//A call to setResolution should fix this
	a->setCamRes(2592,1944);
//	a.setCamRes(320,240);
	if (!ret) {
		bl_log_error("Init failed");
		return ret;
	}
	while(cv::waitKey(1)!=27){
		ret = a->requestFrame();
		if (!ret) {
			bl_log_error("requestFrame failed");
			return ret;
		} else {
			cv::imshow("image", a->cv_frame);
		}
	}
	ret = a->deinit();
	if (!ret) {
		bl_log_error("deinit failed");
		return ret;
	}

	return ret;
}
int main(int argc, cons char* argv[])
{
	ArgusCameraPtr A;
	po::options_description desc = ArgusCamera::GetOptions();
        desc.add_options()
            ("help,h", "Help screen");
	try
	{
		po::variables_map vm;
		store(parse_command_line(argc,argv,desc),vm);
		notify(vm);
	        if (vm.count("help")) {
                    std::cout << desc << '\n';
		} else {
			run(A,&vm);
		}
	} catch (std::exception &e) {
	        std::cout << "Error: " << e.what() << std::endl;
        	std::cout << desc << std::endl;
	}
}
