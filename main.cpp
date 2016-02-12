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

// Toggle display of images.
#define DISP 1

using namespace cv;

int main(int argc, char** argv )
{
    Mat src, src_gray, dst, kern;

    // Check args
    if (argc != 2) {
        printf("usage: DisplayImage.out <Image_Path>\n");
        return -1;
    }

    // Load image
    src = imread(argv[1], CV_LOAD_IMAGE_COLOR);
    src_gray = imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE);
    if (!src.data) {
        printf("No image data \n");
        return -1;
    }
    assert(src.depth() == CV_8U);
    assert(src.channels() == 3);

    // Convert to grayscale
    rgb2g(src, dst);

    // Compare opencv grayscale to our own grayscale
    unsigned int diff = sumOfAbsoluteDifferences(src_gray, dst);
    DLOG("abs diff %u\n", diff);

    // Create kernel and apply to input image.
    kern = (Mat_<char>(3, 3) <<
        0, -1, 0,
        -1, 5, -1,
        0, -1, 0);
    //applyKernel(src, dst, kern);



    if (DISP) {
        // Display original image.
        namedWindow("Source Image", WINDOW_AUTOSIZE );
        imshow("Source Image", src);

        // Display OpenCV grayscale image.
        namedWindow("OpenCV Gray Image", WINDOW_AUTOSIZE );
        imshow("OpenCV Gray Image", src_gray);

        // Display grayscale image.
        namedWindow("Gray Image", WINDOW_AUTOSIZE );
        imshow("Gray Image", dst);

        waitKey(0);
    }

    return 0;
}
