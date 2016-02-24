/**
 * Collection of utility functions.
 *
 * @file utils.cpp
 * @author Emily Ng
 * @date Feb 15 2016
 */

#include <stdarg.h>
#include <stdio.h>

#include "debug.h"
#include "utils.h"

/**
 * Display row of images side by side.
 *
 * Calls to this method move a global position forward, so that subsequent rows
 * of images appear below the current one.

 * @param window_name   Name for image window.
 * @param n             Number of images to show.
 * @param ...           Pointers to Mat objects to be shown.
 */
void displayImageRow(const char *window_name, int n, ...)
{
    if (!DISP) return;

    va_list args;
    va_start(args, n);

    char buf[256];
    int X_INC = 0;
    int Y_INC = 0;

    for (int i = 0; i < n; i++) {
        Mat *img = va_arg(args, Mat*);

        if (img->cols == 0 && img->rows == 0) {
            WLOG("Attempting to display empty image, skipping.");
            break;
        }

        int y_inc = img->rows + 3 * PADDING;
        X_INC = img->cols + PADDING;
        Y_INC = (y_inc > Y_INC) ? y_inc : Y_INC;

        strcpy(buf, window_name);
        sprintf(buf+strlen(buf), " %d", i);

        namedWindow(buf, WINDOW_AUTOSIZE );
        Mat I = *img;
        imshow(buf, I);
        moveWindow(buf, x, y);
        x += X_INC;
    }

    x = 0;
    y += Y_INC;

    va_end(args);

    waitKey(0);

    return;
}
