/**
 * OpenCV prototype of edge detection.
 *
 * @file main.c
 * @author Emily Ng
 * @date Feb 11 2016
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>

#include "debug.h"
#include "img_proc.h"
#include "kernel.h"
#include "utils.h"

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
    Mat dst_x, dst_y;
    // NB: Applying kernel without changing to signed depth (e.g. CV_16S)
    // causes Sobel operator to miss all right edges.
    applyKernel(src, dst_x, kern_sobel_x);
    applyKernel(src, dst_y, kern_sobel_y);
    combine(dst_x, dst_y, dst_filter, &hypoteneuse);
    threshold(dst_filter, dst_filter, 150, 255, THRESH_BINARY);

    Mat tmp_x, tmp_y;
    filter2D(src, tmp_x, CV_16S, kern_sobel_x);
    filter2D(src, tmp_y, CV_16S, kern_sobel_y);
    convertScaleAbs(tmp_x, tmp_x);
    convertScaleAbs(tmp_y, tmp_y);
    addWeighted(tmp_x, 0.5, tmp_y, 0.5, 0, src_filter);
    threshold(src_filter, src_filter, 150, 255, THRESH_BINARY);

    // Compare opencv filter to our own filter
    diff = sumOfAbsoluteDifferences(src_filter, dst_filter);
    DLOG("filter abs diff %u\n", diff);


    // Display results
    displayImagePair("Source", src, src);
    displayImagePair("Gray", src_gray, dst_gray);
    displayImagePair("Filter", src_filter, dst_filter);

    waitKey(0);

    return 0;
}
