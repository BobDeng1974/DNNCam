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
```

Includes Cooper's method for getting frames from argus camera.

Controls need to be tested.

Error logging and handling done to some extent.

TODO:
1. Support for various resolutions supported by driver
2. ROI support
3. Controls settable via xmlrpc
4. integrate existing lens control driver with this source and add test support for xmlrpc
5. error logging and handling
6. Add wiki document
