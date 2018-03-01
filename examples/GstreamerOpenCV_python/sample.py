#!/usr/bin/env python
# MIT License

import sys
import cv2
import numpy as np

def read_cam():
    # On versions of L4T previous to L4T 28.1, flip-method=2
    # Use the Jetson onboard camera
    cap = cv2.VideoCapture("nvcamerasrc ! video/x-raw(memory:NVMM), width=(int)1920, height=(int)1080,format=(string)I420, framerate=(fraction)30/1 ! nvvidconv flip-method=0 ! video/x-raw, format=(string)BGRx ! videoconvert ! video/x-raw, format=(string)BGR ! appsink")
    if cap.isOpened():
    
      #DO PYTHON OPENCV OPERATIONS HERE
              
    else:
     print "camera open failed"



if __name__ == '__main__':
    read_cam()
