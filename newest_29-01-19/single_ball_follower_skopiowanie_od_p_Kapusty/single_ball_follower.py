# parts of the code are based on https://www.pyimagesearch.com/2016/01/04/unifying-picamera-and-cv2-videocapture-into-a-single-class-with-opencv/
# before running the code install imutils for python3 (pip3 install imutils)

import time
from imutils.video import VideoStream
import serial
# from picamera.array import PiRGBArray
# from picamera import PiCamera
import numpy as np
import cv2

def add_50_to_coord(x, edge):
    diff = edge - x + 50
    if diff < 0:
        new_x = x + (50 - diff)
    else:
        new_x = x + 50
    return new_x


def substract_50_from_coord(x):
    if x - 50 < 0:
        new_x = 0
    else:
        new_x = x - 50
    return new_x


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
a_button_clicked = False

# # initialize serial communication
ser = serial.Serial(port='/dev/ttyS0', baudrate=9600, timeout=0.05)



while True:
# for cameraFrame in camera.capture_continuous(rawCapture, format="bgr", use_video_port=True):
    #print('lop')
    loopStart = time.time()
    if not paused:

        frame = vs.read()
        frame = cv2.flip(frame, flipCode=-1)
        
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
        contour_list = []
        
        #contour_list = contours
        
        
        if a_button_clicked:
            
            rect = cv2.boundingRect(mask)
            
            #print(rect)
            # [0] -> Y, [1] -> X ; jest na odwrot
            # change 50 for edges
            y_lower = substract_50_from_coord(scaleFactor*rect[1])
            y_upper = add_50_to_coord(scaleFactor*rect[1]+scaleFactor*rect[3], height)
            x_lower = substract_50_from_coord(scaleFactor*rect[0])
            x_upper = add_50_to_coord(scaleFactor*rect[0]+scaleFactor*rect[2], width)
            #print(height, width)
            #print(y_lower, y_upper)
            #print(x_lower, x_upper)
            crop_img = frame[y_lower:y_upper, x_lower:x_upper]
            #cv2.imshow("crop_img", crop_img)
            output = crop_img.copy()
            cv2.imshow("output", output)
            
            # is not working with canny at all?
            #blur = crop_img
            blur = cv2.GaussianBlur(crop_img, (5,5), 0)
            cv2.imshow("blur", blur)
            gray_cropped = cv2.cvtColor(blur, cv2.COLOR_BGR2GRAY)
            #canny = cv2.Canny(gray_cropped, 50, 200, 10)
            #cv2.imshow("canny", canny)
            circles = cv2.HoughCircles(gray_cropped, cv2.HOUGH_GRADIENT, 1, 1000, param1=100, param2=30, minRadius=15, maxRadius=80)
            # ensure at least some circles were found
            #print(circles)
            #print ("loop2")
            #cv2.imwrite("yy.jpg", crop_img)
            
            if circles is not None:
                
                # convert the (x, y) coordinates and radius of the circles to integers
                circles = np.round(circles[0, :]).astype("int")
         
                # loop over the (x, y) coordinates and radius of the circles
                for (x, y, r) in circles:
                        # draw the circle in the output image, then draw a rectangle
                        # corresponding to the center of the circle
                        cv2.circle(output, (x, y), r, (0, 255, 0), 4)
                        cv2.rectangle(output, (x - 5, y - 5), (x + 5, y + 5), (0, 128, 255), -1)
         
                # show the output image
                cv2.imshow("output", output)
                #print("here")
                #cv2.waitKey(0)
                
                for contour in contours:
                    approx = cv2.approxPolyDP(contour, 0.01*cv2.arcLength(contour, True), True)
                    area = cv2.contourArea(contour)
                    #print(len(approx))
                    if ((len(approx) > 12) and (area > 30)):
                        contour_list.append(contour)
                
        
        """
                #http://layer0.authentise.com/detecting-circular-shapes-using-contours.html            
        for contour in contours:
            approx = cv2.approxPolyDP(contour, 0.01*cv2.arcLength(contour, True), True)
            #print("approx")
            #print(len(approx))
            #print(approx)
            area = cv2.contourArea(contour)
            if ((len(approx) > 14) and (area > 30)):
                #print(approx)
                #approx = approx.reshape(len(approx), 2)
                #print(approx)
                #min_value_x, min_value_y = np.min(approx, axis=0)
                #max_value_x, max_value_y = np.max(approx, axis=0)
                
                #print(min_value_x)
                #print(min_value_y)
                #print(max_value_x)
                #print(max_value_y)
                #crop_img = frame[min_value_x:max_value_x, min_value_y:max_value_y]
                
                #cropped_resizedColor = cv2.resize(crop_img, (newWidth, newHeight), interpolation=cv2.INTER_CUBIC)
                #cropped_resizedColor_blurred = cv2.GaussianBlur(cropped_resizedColor, (5, 5), 0)
                #cropped_resizedHSV = cv2.cvtColor(cropped_resizedColor_blurred, cv2.COLOR_BGR2HSV)
                
                #cv2.imshow("resizedHSV", cropped_resizedHSV)
                contour_list.append(contour)
          """

        boundingBoxes = []
        biggestObject_BoundingBox = None
        biggestObjectMiddle = None
        #print("contour_list" + str(contour_list))
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
        #print("biggestObject_BoundingBox" + str(biggestObject_BoundingBox))
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
            print(str(yaw) + "    " + str(pitch))
            packet = '<packet, {}, {}>'.format(yaw, pitch)
            packetBytes = bytes(packet, 'utf-8')
            
            ser.write(packetBytes)
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
        a_button_clicked = True
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
        colorLower = (max(0,avg_h), max(0, avg_s - 10), max(0,avg_v - 10))
        colorUpper = (min (255, avg_h), min(255, avg_s + 80), min(255, avg_v + 80))
    elif key == ord('z'):
        h = roi[:,:,0]
        s = roi[:,:,1]
        v = roi[:,:,2]
        colorLower = (int(np.min(h)), max(0, int(np.min(s))), max(0, int(np.min(v))))
        colorUpper = (int(np.max(h)), min(255, int(np.max(s))), min(255, int(np.max(v))))
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
    #print("loop execution took {:3.2f}ms".format((loopEnd - loopStart)*1000))
    
# cleanup
cv2.destroyAllWindows()
vs.stop()
