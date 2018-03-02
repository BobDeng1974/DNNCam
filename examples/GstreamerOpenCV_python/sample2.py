import numpy as np
import cv2

cap = cv2.VideoCapture("nvcamerasrc ! video/x-raw(memory:NVMM), width=(int)1280, height=(int)720, format=(string)I420, framerate=(fraction)24/1 ! nvvidconv flip-method=6 ! video/x-raw, format=(string)I420 ! videoconvert ! video/x-raw, format=(string)BGR ! appsink")
#cap=cv2.VideoCapture(0)
#cap=cv2.VideoCapture("/dev/video0")

if not cap.isOpened() :
	print("not capture")
	exit()
ret,frame = cap.read()
while ret :
	cv2.imshow('frame',frame)
	ret,frame = cap.read()
	if(cv2.waitKey(1) & 0xFF == ord('q')):
		break;

cap.release()
cv2.destroyAllWindows()
