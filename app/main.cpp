#include <iostream>

#include <include/ArgusCamera.hpp>
#include <include/log.hpp>

int main()
{
	bool ret = true;
	std::vector<float> wb = {1.0,1.0,1.0,1.0};
	//device_id, Width, Height, Min Exposure, Max exposure, Min Gain, Max Gain, AWB mode, AWB Gains, fps, timeout, exposure comp
	ArgusCamera a(0,320,240,0.003,0.033,1.0,16.0,AutoWhiteBalanceMode::AUTO, wb,29.99,1.0,0.0);

//	std::ofstream outfile("test", std::ofstream::binary);
	ret = a.init();
	//TODO: This changes the camera resolution but the argus output resolution is configured in init().
	//NvBuffer is allocated during requestFrame(). 
	//Hence the output is resized from argus output resolution to NvBuffer resolution
	//A call to setResolution should fix this
	a.setCamRes(2592,1944);
//	a.setCamRes(320,240);
	if (!ret) {
		bl_log_error("Init failed");
		return ret;
	}
	while(cv::waitKey(1)!=27){
		ret = a.requestFrame();
		if (!ret) {
			bl_log_error("requestFrame failed");
			return ret;
		} else {
//			std::cout << a.cv_frame.data << std::endl;
//			outfile.write ((const char*) a.cv_frame.data,1948*1096*4);
//			outfile.close();
			cv::imshow("image", a.cv_frame);
		}
	}
	ret = a.deinit();
	if (!ret) {
		bl_log_error("deinit failed");
		return ret;
	}

	return ret;
}
