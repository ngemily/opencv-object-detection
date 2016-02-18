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

    /*****      Convert to grayscale     *******/
    Mat src_gray, dst_gray;

    rgb2g(src, dst_gray);                       // ours
    cvtColor(src, src_gray, CV_BGR2GRAY, 0);    // OpencV

    // Compare
    unsigned int diff = sumOfAbsoluteDifferences(src_gray, dst_gray);
    DLOG("gray abs diff %u\n", diff);

    /*****      Sobel edge detection     *******/
    Mat src_filter, dst_filter;

    // Ours
    Mat dst_x, dst_y;
    applyKernel(src, dst_x, kern_sobel_x);
    applyKernel(src, dst_y, kern_sobel_y);
    combine(dst_x, dst_y, dst_filter, &hypoteneuse);
    threshold(dst_filter, dst_filter, 150, 255, THRESH_BINARY);

    // OpenCV
    Mat tmp_x, tmp_y;
    filter2D(src, tmp_x, CV_16S, kern_sobel_x);     // x derivative
    filter2D(src, tmp_y, CV_16S, kern_sobel_y);     // y derivative
    addWeighted(tmp_x, 0.5, tmp_y, 0.5, 0, src_filter); // combine
    convertScaleAbs(src_filter, src_filter);        // back to CV_8U
    threshold(src_filter, src_filter, 150, 255, THRESH_BINARY);

    // Compare
    diff = sumOfAbsoluteDifferences(src_filter, dst_filter);
    DLOG("filter abs diff %u\n", diff);

    /*****      Isolate objects     *******/
    Mat src_obj;    // grayscale and binarize output from edge detection
    Mat dst_obj;    // for debug, draw bounding boxes around objects
    Mat obj[10];    // put each object in its own Mat

    cvtColor(dst_filter, src_obj, CV_BGR2GRAY, 0);
    threshold(src_obj, src_obj, 50, 255, THRESH_BINARY);
    dst_obj = src_obj.clone();

    for (int i = 0; i < 10; i++) {
        struct rect r = extractObject(src_obj, dst_obj);
        obj[i] = src(Range(r.top, r.bottom), Range(r.left, r.right));
        DLOG("obj %d is %d x %d\n", i, obj[i].cols, obj[i].rows);
    }

    /*****      Image moments     *******/
    // Ours
    _moment _m = imageMoments(src_obj);
    double *_hu = (double *)&_m.hu;

    // OpenCV
    double hu[7];
    Moments m = moments(src_obj, false);
    HuMoments(m, hu);



    /*
    DLOG("Image moments\n");
    DLOG("%10s %12s %12s\n", "", "OpenCV", "custom");
    DLOG("%10s %12.0f %12.0f\n", "m00", m.m00, _m.m00);
    DLOG("%10s %12.3f %12.3f\n", "xbar", m.m01 / m.m00, _m.m01 / _m.m00);
    DLOG("%10s %12.3f %12.3f\n", "ybar", m.m10 / m.m00, _m.m10 / _m.m00);
    DLOG("\n");
    DLOG("%10s %12e %12e\n", "u02", m.mu02, _m.u02);
    DLOG("%10s %12e %12e\n", "u03", m.mu03, _m.u03);
    DLOG("%10s %12e %12e\n", "u11", m.mu11, _m.u11);
    DLOG("%10s %12e %12e\n", "u12", m.mu12, _m.u12);
    DLOG("%10s %12e %12e\n", "u21", m.mu21, _m.u21);
    DLOG("%10s %12e %12e\n", "u20", m.mu20, _m.u20);
    DLOG("%10s %12e %12e\n", "u30", m.mu30, _m.u30);
    DLOG("\n");
    DLOG("%10s %12.8f %12.8f\n", "u02", m.nu02, _m.n02);
    DLOG("%10s %12.8f %12.8f\n", "u03", m.nu03, _m.n03);
    DLOG("%10s %12.8f %12.8f\n", "u11", m.nu11, _m.n11);
    DLOG("%10s %12.8f %12.8f\n", "u12", m.nu12, _m.n12);
    DLOG("%10s %12.8f %12.8f\n", "u21", m.nu21, _m.n21);
    DLOG("%10s %12.8f %12.8f\n", "u20", m.nu20, _m.n20);
    DLOG("%10s %12.8f %12.8f\n", "u30", m.nu30, _m.n30);
    DLOG("\n");
    DLOG("%10s %12.8f %12.8f\n", "hu[0]", hu[0], _hu[0]);
    DLOG("%10s %12.8f %12.8f\n", "hu[1]", hu[1], _hu[1]);
    DLOG("%10s %12.8f %12.8f\n", "hu[2]", hu[2], _hu[2]);
    DLOG("%10s %12.8f %12.8f\n", "hu[3]", hu[3], _hu[3]);
    DLOG("%10s %12.8f %12.8f\n", "hu[4]", hu[4], _hu[4]);
    DLOG("%10s %12.8f %12.8f\n", "hu[5]", hu[5], _hu[5]);
    DLOG("%10s %12.8f %12.8f\n", "hu[6]", hu[6], _hu[6]);
    */


    DLOG("Hu moments\n");
    DLOG("%10s %12s %12s\n", "", "OpenCV", "custom");
    for (int i = 0; i < 7; i++) {
        DLOG("%10s %12.8f %12.8f\n", "hu[i]", hu[i], _hu[i]);
    }
    printf("\n");

    // Display results
    displayImageRow("Extract obj", 4, &src, &obj[0], &obj[1], &obj[2]);
    //displayImageRow("Source", src, src);
    //displayImageRow("Gray", src_gray, dst_gray);
    //displayImageRow("Filter", src, dst_filter);

    return 0;
}
