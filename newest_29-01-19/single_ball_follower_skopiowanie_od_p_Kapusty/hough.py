import numpy as np
import cv2

crop_img = cv2.imread("x.jpg")
#print(image)
cv2.imshow("crop_img", crop_img)
#cv2.waitKey(0)
output = crop_img.copy()

# blur, canny, params

gray_cropped = cv2.cvtColor(crop_img, cv2.COLOR_BGR2GRAY)
cv2.imshow("gray_cropped", gray_cropped)

# circles = cv2.HoughCircles(gray, cv2.cv.CV_HOUGH_GRADIENT, 1.2, 100)
circles = cv2.HoughCircles(gray_cropped, cv2.HOUGH_GRADIENT, 1, 10, param2=50, minRadius=0)
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
cv2.waitKey(0)
