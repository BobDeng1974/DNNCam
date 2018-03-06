# DNNCam


![alt text](https://github.com/BoulderAI/DNNCam/blob/master/dnnhero_2a.png?raw=true "Logo Title Text 1")


From jetpack_download directory scp \*arm64\*.deb onto the TX2.

Then on the TX2 run
```
sudo dpkg -i cuda-repo-l4t-8-0-local_8.0.64-1_arm64.deb
dpkg -i libopencv4tegra-repo_2.4.13-17-g5317135_arm64_l4t-r24.deb
sudo apt-get update
sudo apt-get install libopencv4tegra-dev

sudo add-apt-repository universe
sudo apt-get install libgstreamer1.0-dev
sudo apt-get install libboost-all-dev
sudo apt-get install libgstrtspserver-1.0-dev
sudo apt-get install libarchive-dev
sudo apt-get install libxmlrpc-c++8-dev
sudo apt-get install libv4l-dev
sudo apt-get install libreadline-dev
sudo apt-get install libi2c-dev i2c-tools
sudo apt-get install cmake
```

To build DNNCam, from the top level source directory:
```
mkdir build
cmake ..
make -j6
```

If you want to install to /usr/local/dnncam and put the web files in
/var/www, from your build directory:
```
sudo make install
```

If you want to run the webserver:
```
sudo apt-get install lighttpd
cd /etc/lighttpd/conf-enabled
sudo ln -s /etc/lighttpd/conf-available/10-cgi.conf .
cd /var/www/html
sudo ln -s /usr/lib/cgi-bin .
```

Config File:

Any of the command line parameters may be placed in a config file in
/etc/dnncam.conf. If you would like to add this to your code, look at
how the file is loaded and parsed in parse_arguments() in
camerastreamer.cpp

XMLRPC Interface:

By default, the camerastreamer program runs an XMLRPC server for
remote setting of camera parameters. Here is a basic list of the
XMLRPC controls available:

Image Controls:
```
void set_auto_exposure(bool) - Sets auto exposure (auto exposure "lock")
bool get_auto_exposure(void) - Gets auto exposure
Array(i8, i8) get_exposure_time(void) - Gets the exposure time min and max
void set_exposure_time(i8, i8) - Sets the exposure time min and max
void set_exposure_compensation(double) - Sets the exposure compensation
double get_exposure_compensation(void) - Gets the exposure compensation
void set_frame_duration(i8, i8) - Sets the frame duration time min and max
Array(i8, i8) get_frame_duration(void) - Gets the frame duration time min and max
void set_gain(double, double) - Sets the gain min and max
Array(double, double) get_gain(void) - Gets the gain min and max
void set_awb(bool) - Sets AWB state
bool get_awb(void) - Gets AWB state
string get_awb_mode(void) - Gets AWB mode
void set_awb_mode(string) - Sets AWB mode
void set_awb_gains(double, double, double, double) - Sets AWB gains
Array(double, double, double, double) get_awb_gains(void) - Gets AWB gains
string get_denoise_mode(void) - Gets denoise mode
void set_denoise_mode(string) - Sets denoise mode
void set_denoise_strength(double) - Sets denoise strength
double get_denoise_strength(void) - Gets denoise strength
string get_config(void) - Returns a chunk of table HTML that shows the camera settings
```

Lens Controls (NOTE: at the time of this writing, the limit switches
were not working, and the *_absolute(), *_home(), and *_get_location()
functions do not work)
```
int focus_home(void) - Sets focus to the 'home' location
int focus_absolute(int) - Sets focus to the given absolute location
int focus_relative(int) - Sets focus relative to the current position
int focus_get_location(void) - Get the absolute focus location
int zoom_home(void) - Sets zoom to the 'home' location
int zoom_absolute(int) - Sets zoom to the given absolute location
int zoom_relative(int) - Sets zoom relative to the current position
int zoom_get_location(void) - Get the absolute zoom location
int iris_home(void) - Sets iris to the 'home' location
int iris_absolute(int) - Sets iris to the given absolute position
int iris_relative(int) - Sets iris relative to the current position
int iris_get_location(void) - Get the absolute iris location
int ir_cut(bool) - Set the IR cut filter
```
