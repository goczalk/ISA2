# parts of the code are based on https://www.pyimagesearch.com/2016/01/04/unifying-picamera-and-cv2-videocapture-into-a-single-class-with-opencv/
# before running the code install imutils for python3 (pip3 install imutils)

import time
from imutils.video import VideoStream
import serial
# from picamera.array import PiRGBArray
# from picamera import PiCamera
import numpy as np
import cv2


def translate(value, oldMin, oldMax, newMin=-100, newMax=100):
    # Figure out how 'wide' each range is
    oldRange = oldMax - oldMin
    newRange = newMax - newMin
    NewValue = (((value - oldMin) * newRange) / oldRange) + newMin
    return int(NewValue)

# False when on computer
usesPiCamera = True

# camera = PiCamera()
# camera.framerate = 60
cameraResolution = (640, 480)
# camera.resolution = cameraResolution

# # camera.awb_mode = 'tungsten'
# camera.vflip = camera.hflip = True
# camera.video_stabilization = True
# rawCapture = PiRGBArray(camera, size=cameraResolution)

# initialize the video stream and allow the cammera sensor to warmup
vs = VideoStream(usePiCamera=usesPiCamera, resolution=cameraResolution, framerate=60).start()
time.sleep(2.0)

colorLower = (0, 100, 50)
colorUpper = (100, 255, 255)
colorTolerance = 3
paused = False
roiSize = (16, 16) # roi size on the scaled down image (converted to HSV)


# # initialize serial communication
# ser = serial.Serial(port='/dev/ttyACM0', baudrate=57600, timeout=0.05)

while True:
# for cameraFrame in camera.capture_continuous(rawCapture, format="bgr", use_video_port=True):
    loopStart = time.time()
    if not paused:

        frame = vs.read()
        # frame = cv2.flip(frame, flipCode=-1)
        
        height, width = frame.shape[0:2]
        scaleFactor = 4
        newWidth, newHeight = width//scaleFactor, height//scaleFactor


        resizedColor = cv2.resize(frame, (newWidth, newHeight), interpolation=cv2.INTER_CUBIC)
        resizedColor_blurred = cv2.GaussianBlur(resizedColor, (5, 5), 0)

        # resizedHSV = cv2.cvtColor(resizedColor, cv2.COLOR_BGR2HSV)
        resizedHSV = cv2.cvtColor(resizedColor_blurred, cv2.COLOR_BGR2HSV)

        roi = resizedHSV[newHeight//2 - roiSize[0]//2 : newHeight //2 + roiSize[0]//2, newWidth//2 - roiSize[1]//2 : newWidth//2 + roiSize[1]//2, :]
        # roi = resizedHSV[10*newHeight//20 : 12*newHeight//20, 10*newWidth//20 : 12*newWidth // 20, :]
        
        colorLowerWithTolerance = (colorLower[0] - colorTolerance,) + colorLower[1:]
        colorUpperWithTolerance = (colorUpper[0] + colorTolerance,) + colorUpper[1:]

        mask = cv2.inRange(resizedHSV, colorLowerWithTolerance, colorUpperWithTolerance)
        cv2.erode(mask, None, iterations=5)
        cv2.dilate(mask, None, iterations=5)

        (_, contours, hierarchy) = cv2.findContours(mask, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)

        # circle?
        # tak wyglada zwracanie approx, sortuj lub max i min wyciagnij
        """
[[[79 24]]

 [[73 24]]

 [[71 28]]

 [[67 28]]

 [[61 32]]

 [[61 39]]

 [[58 42]]

 [[59 50]]

 [[66 57]]

 [[60 49]]

 [[63 43]]

 [[72 46]]

 [[82 46]]

 [[90 50]]

 [[92 46]]

 [[90 33]]]

        """
        #http://layer0.authentise.com/detecting-circular-shapes-using-contours.html
        contour_list = []
        for contour in contours:
            approx = cv2.approxPolyDP(contour, 0.01*cv2.arcLength(contour, True), True)
            print("approx")
            print(len(approx))
            #print(approx)
            area = cv2.contourArea(contour)
            if ((len(approx) > 14) and (area > 30)):
                print(approx)
                # more conditions
                #crop_img = resizedHSV[y:y+h, x:x+w]
                #print(approx[0][0])
                min_coord_x, min_coord_y = approx[0][0]
                #print(approx[0])
                #print(min_coord_x)
                #print(min_coord_y)
                max_coord_x, max_coord_y = approx[-1][0]
                
                #crop_img = resizedHSV[min_coord_x:max_coord_x, min_coord_y:max_coord_y]
                cv2.imshow("resizedHSV", resizedHSV)
                #cv2.imshow("crop_img", crop_img)
                contour_list.append(contour)


        boundingBoxes = []
        biggestObject_BoundingBox = None
        biggestObjectMiddle = None
        if contour_list:
        #if contours:
            largestContour = max(contour_list, key=cv2.contourArea)
            #largestContour = max(contours, key=cv2.contourArea)
            biggestObject_BoundingBox = cv2.boundingRect(largestContour)
            
            for i, contour in enumerate(contour_list):
            #for i, contour in enumerate(contours):
                area = cv2.contourArea(contour)
                if area > ((newWidth * newHeight)/256):
                    x,y,w,h = cv2.boundingRect(contour)
                    boundingBoxes.append((x,y,w,h))
                    # print("Object {}: ({},{}); {}x{}; area: {}".format(i, x,y,w,h, area))
        else:
            pass

        upscaledColor = cv2.resize(resizedColor, (width, height), interpolation=cv2.INTER_NEAREST)
        # draw ROI on upscaled image
        xROI, yROI = width//2 - roiSize[1]//2 * scaleFactor, height//2 - roiSize[0]//2 * scaleFactor
        cv2.rectangle(upscaledColor, (xROI, yROI), (xROI + roiSize[0]*scaleFactor, yROI + roiSize[1]*scaleFactor), (0, 0, 0), thickness=3)

        for boundingBox in boundingBoxes:
            x,y,w,h = boundingBox
            cv2.rectangle(resizedColor, (x, y), (x+w, y+h), (255, 255, 0), thickness=1)
            cv2.rectangle(upscaledColor, (x*scaleFactor, y*scaleFactor),
                        ((x+w)*scaleFactor, (y+h)*scaleFactor), (255, 255, 0), thickness=2)
        
        if biggestObject_BoundingBox:
            x, y, w, h = biggestObject_BoundingBox
            biggestObjectMiddle = ((x+ w//2)*scaleFactor, (y + h//2)*scaleFactor)
            cv2.rectangle(resizedColor, (x, y), (x+w, y+h), (0, 0, 255), thickness=2)
            cv2.rectangle(upscaledColor, (x*scaleFactor, y*scaleFactor),
                            ((x+w)*scaleFactor, (y+h)*scaleFactor), (0, 0, 255), thickness=3)
            cv2.circle(upscaledColor, biggestObjectMiddle, 2, (255, 0, 0), thickness=2)
            screenMiddle = width//2, height//2
            distanceVector = tuple(map(lambda x, y: x - y, biggestObjectMiddle, screenMiddle))
            # print("Vector: {}".format(distanceVector))
            scaled = (translate(distanceVector[0], -width//2, width//2), translate(distanceVector[1], -height//2, height//2) )
            # print("Vector scaled: {}".format(scaled))
            pitch = scaled[1] # up-down
            yaw = scaled[0] # left-right
            cv2.line(upscaledColor, screenMiddle, biggestObjectMiddle, (0, 0, 255))
            packet = '<packet, {}, {}>'.format(yaw, pitch)
            packetBytes = bytes(packet, 'utf-8')
            
            # ser.write(packetBytes)
            # print(ser.read_all())
            

        cv2.imshow("video", upscaledColor)
        cv2.imshow("roi", roi)
        cv2.imshow("mask", mask)

        modTolerances = False

    # handle keys 
    key = cv2.waitKey(1) & 0xFF
    if key == ord('q'):
        break
    elif key == ord('a'):
        avg_h = 0
        avg_s = 0
        avg_v = 0
        i = 0
        for _, row in enumerate(roi):
            avg = np.average(row, 0)
            avg_h += avg[0]
            avg_s += avg[1]
            avg_v += avg[2]
            i+=1

        avg_h /= i
        avg_s /= i
        avg_v /= i
        print("HUE:{}, SAT:{}, VAL:{}".format(avg_h, avg_s, avg_v))
        colorLower = (max(0,avg_h), max(0, avg_s - 50), max(0,avg_v - 50))
        colorUpper = (min (255, avg_h), min(255, avg_s + 50), min(255, avg_v + 50))
    elif key == ord('z'):
        h = roi[:,:,0]
        s = roi[:,:,1]
        v = roi[:,:,2]
        colorLower = (int(np.min(h)), max(0, int(np.min(s)-20 )), max(0, int(np.min(v)-20)))
        colorUpper = (int(np.max(h)), min(255, int(np.max(s)+20)), min(255, int(np.max(v)+20)))
    elif key == ord('w'):
        colorTolerance = min(colorTolerance + 1, 50)
        print("New color range: {}".format(colorTolerance))
    elif key == ord('s'):
        colorTolerance = max(colorTolerance - 1, 0)
        print("New color range: {}".format(colorTolerance))
    elif key == ord('p'):
        paused = not paused
    elif key == ord('d'):
        # pause/unpause arduino camera movement
        ser.write(bytes('d', 'utf-8'))
    
    # rawCapture.truncate(0)
    loopEnd= time.time()
    print("loop execution took {:3.2f}ms".format((loopEnd - loopStart)*1000))
    
# cleanup
cv2.destroyAllWindows()
vs.stop()
