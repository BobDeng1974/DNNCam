import numpy as np
import cv2
#1932 x 1094 FR=120.000000
cap = cv2.VideoCapture("nvcamerasrc ! video/x-raw(memory:NVMM), width=(int)1932, height=(int)1094, format=(string)I420, framerate=(fraction)30/1 ! nvvidconv flip-method=6 ! video/x-raw, format=(string)I420 ! videoconvert ! video/x-raw, format=(string)BGR ! appsink")


# Define the codec and create VideoWriter object
fourcc = cv2.VideoWriter_fourcc(*'H264')
out = cv2.VideoWriter('output.mp4',fourcc, 30.0, (1932,1094))
#out = cv2.VideoWriter('output.avi', -1, 20.0, (640,480))

while(cap.isOpened()):
    ret, frame = cap.read()
    if ret==True:
        frame = cv2.flip(frame,0)

       # write the flipped frame
        out.write(frame)

       #cv2.imshow('frame',frame)
        #if cv2.waitKey(1) & 0xFF == ord('q'):
        #    break
    else:
        break

# Release everything if job is finished
cap.release()
out.release()
cv2.destroyAllWindows()
