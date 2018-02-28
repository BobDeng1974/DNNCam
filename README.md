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
