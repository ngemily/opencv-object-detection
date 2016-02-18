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
    applyKernel(src, dst_x, kern_sobel_x);
    applyKernel(src, dst_y, kern_sobel_y);
    combine(dst_x, dst_y, dst_filter, &hypoteneuse);
    threshold(dst_filter, dst_filter, 150, 255, THRESH_BINARY);

    // OpenCV Sobel
    Mat tmp_x, tmp_y;
    filter2D(src, tmp_x, CV_16S, kern_sobel_x);     // x derivative
    filter2D(src, tmp_y, CV_16S, kern_sobel_y);     // y derivative
    addWeighted(tmp_x, 0.5, tmp_y, 0.5, 0, src_filter); // combine
    convertScaleAbs(src_filter, src_filter);        // back to CV_8U
    threshold(src_filter, src_filter, 150, 255, THRESH_BINARY);

    // Isolate objects
    Mat tmp, src_obj, dst_obj;
    double hu[7];

    cvtColor(dst_filter, src_obj, CV_BGR2GRAY, 0);
    threshold(src_obj, src_obj, 50, 255, THRESH_BINARY);
    Moments m = moments(src_obj, false);
    HuMoments(m, hu);

    tmp = src_obj.clone();
    DLOG("%.0f %.0f %.0f\n", m.m00, m.m01 / m.m00, m.m10 / m.m00);

    dst_obj = src_obj.clone();
    for (int i = 0; i < 20; i++) {
        extractObject(src_obj, dst_obj);
    }

    for (int i = 0; i < 7; i++) {
        printf("%f ", hu[i]);
    }
    printf("\n");

    // Compare opencv filter to our own filter
    diff = sumOfAbsoluteDifferences(src_filter, dst_filter);
    DLOG("filter abs diff %u\n", diff);


    // Display results
    displayImagePair("BB", src_obj, dst_obj);
    //displayImagePair("Source", src, tmp);
    //displayImagePair("Gray", src_gray, dst_gray);
    displayImagePair("Filter", src, dst_filter);

    waitKey(0);

    return 0;
}
