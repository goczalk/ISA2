import numpy as np
import cv2

crop_img = cv2.imread("y.jpg")
crop_img2 = cv2.imread("yy.jpg")
cv2.imshow("crop_img", crop_img)
cv2.imshow("crop_img", crop_img2)
#cv2.waitKey(0)
output = crop_img.copy()
output2 = crop_img2.copy()

# blur, canny, params

blur = cv2.GaussianBlur(crop_img, (5,5), 0)

blur2 = cv2.GaussianBlur(crop_img2, (5,5), 0)
cv2.imshow("blur", blur)
cv2.imshow("blur", blur2)
gray_cropped = cv2.cvtColor(blur, cv2.COLOR_BGR2GRAY)

gray_cropped2 = cv2.cvtColor(blur2, cv2.COLOR_BGR2GRAY)
#canny = cv2.Canny(gray_cropped, 50, 200, 10)
#cv2.imshow("canny", canny)
circles = cv2.HoughCircles(gray_cropped, cv2.HOUGH_GRADIENT, 1, 1000, param1=100, param2=30, minRadius=15, maxRadius=100)

circles2 = cv2.HoughCircles(gray_cropped2, cv2.HOUGH_GRADIENT, 1, 1000, param1=100, param2=30, minRadius=15, maxRadius=100)
print(circles)

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
    
    
if circles2 is not None:
    # convert the (x, y) coordinates and radius of the circles to integers
    circles2 = np.round(circles2[0, :]).astype("int")

    # loop over the (x, y) coordinates and radius of the circles
    for (x, y, r) in circles2:
            # draw the circle in the output image, then draw a rectangle
            # corresponding to the center of the circle
            cv2.circle(output2, (x, y), r, (0, 255, 0), 4)
            cv2.rectangle(output2, (x - 5, y - 5), (x + 5, y + 5), (0, 128, 255), -1)

    # show the output image
    cv2.imshow("output2", output2)
    
cv2.waitKey(0)
