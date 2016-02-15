/**
 * Collection of utility functions.
 *
 * @file utils.cpp
 * @author Emily Ng
 * @date Feb 15 2016
 */

#include "utils.h"

/**
 * Display pair of images side by side.
 *
 * If only one image is to be displayed, pass the same image twice.  Calls to
 * this method move a global position forward, so that subsequent pairs of
 * images appear below the current one.  The image on the right will have a
 * suffix appended to the window name.
 
 * Main purpose is two show result of two different implementations of the same
 * image processing algorithm, usually OpenCV library vs hand written.
 *
 * @param window_name   Name for image window.
 * @param img1          Image to be shown on the left.
 * @param img2          Image to be shown on the right.
 */
void displayImagePair(const char *window_name, Mat &img1, Mat &img2)
{
    if (!DISP) return;

    const int X_INC = img1.cols + PADDING;
    const int Y_INC = img1.rows + 3 * PADDING;

    char buf[256];
    strcpy(buf, "OpenCV ");
    strcat(buf, window_name);

    namedWindow(buf, WINDOW_AUTOSIZE );
    imshow(buf, img1);
    moveWindow(buf, x, y);
    x += X_INC;

    if (img1.data != img2.data) {
        namedWindow(window_name, WINDOW_AUTOSIZE );
        imshow(window_name, img2);
        moveWindow(window_name, x, y);
    }

    x = 0;
    y += Y_INC;

    return;
}
