/**
 * @file main.c
 * @author Emily Ng
 * @date Feb 11 2016
 * @brief OpenCV prototype of edge detection
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>

#include "debug.h"
#include "img_proc.h"
#include "kernel.h"

#define DISP 1          // Toggle display of images.
#define PADDING 20      // Padding between images.

using namespace cv;

int main(int argc, char** argv )
{
    Mat src;                    // Load source image.
    Mat src_gray, src_filter;   // Computed by OpenCV.
    Mat dst_gray, dst_filter;   // Computed by program.

    // Check args
    if (argc != 2) {
        printf("usage: DisplayImage.out <Image_Path>\n");
        return -1;
    }

    // Load image
    src = imread(argv[1], CV_LOAD_IMAGE_COLOR);
    if (!src.data) {
        printf("No image data \n");
        return -1;
    }

    // Convert to grayscale
    rgb2g(src, dst_gray);
    cvtColor(src, src_gray, CV_BGR2GRAY, 0);

    // Compare opencv grayscale to our own grayscale
    unsigned int diff = sumOfAbsoluteDifferences(src_gray, dst_gray);
    DLOG("gray abs diff %u\n", diff);

    // Apply kernel to input image.
    applyKernel(src, dst_filter, kern_sobel_x);
    filter2D(src, src_filter, src.depth(), kern_sobel_x);

    // Compare opencv filter to our own filter
    diff = sumOfAbsoluteDifferences(src_filter, dst_filter);
    DLOG("filter abs diff %u\n", diff);


    // Display results
    const int X_INC = src.cols + PADDING;
    const int Y_INC = src.rows + 3 * PADDING;
    int x=0, y=0;

    if (DISP) {
        // Display original image.
        namedWindow("Source Image", WINDOW_AUTOSIZE );
        imshow("Source Image", src);
        moveWindow("Source Image", x, y);
        y += Y_INC;

        // Display OpenCV grayscale image.
        namedWindow("OpenCV Gray Image", WINDOW_AUTOSIZE );
        imshow("OpenCV Gray Image", src_gray);
        moveWindow("OpenCV Gray Image", x, y);
        x += X_INC;

        // Display grayscale image.
        namedWindow("Gray Image", WINDOW_AUTOSIZE );
        imshow("Gray Image", dst_gray);
        moveWindow("Gray Image", x, y);
        x = 0;
        y += Y_INC;

        // Display OpenCV filtered image.
        namedWindow("OpenCV Filtered Image", WINDOW_AUTOSIZE );
        imshow("OpenCV Filtered Image", src_filter);
        moveWindow("OpenCV Filtered Image", x, y);
        x += X_INC;

        // Display filtered image.
        namedWindow("Filtered Image", WINDOW_AUTOSIZE );
        imshow("Filtered Image", dst_filter);
        moveWindow("Filtered Image", x, y);
        x = 0;
        y += Y_INC;

        waitKey(0);
    }

    return 0;
}
