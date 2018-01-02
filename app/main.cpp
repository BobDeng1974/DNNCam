#include <iostream>

#include <include/ArgusCamera.hpp>

int main()
{
	bool ret = true;
	std::vector<float> wb = {1.0,1.0,1.0,1.0};
	// Width, Height, Min Exposure, Max exposure, Min Gain, Max Gain, AWB mode, AWB Gains, fps, timeout, exposure comp
	ArgusCamera a(1948,1096,0.003,0.033,1.0,1.0,AutoWhiteBalanceMode::AUTO, wb,29.99,1.0,0.0);

	a.init();
	while(cv::waitKey(1)!=27){
		a.requestFrame();
		cv::imshow("image", a.cv_frame);
	}
	//TODO: Need to implement this in library to make sure application quits without crashing argus_daemon
	//a.deinit();

	return 0;
}
