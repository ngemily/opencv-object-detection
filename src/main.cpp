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
        DLOG("usage: DisplayImage.out <Image_Path>");
        return -1;
    }

    // Load image
    src = imread(argv[1], CV_LOAD_IMAGE_COLOR);
    if (!src.data) {
        ELOG("No image data.");
        return -1;
    }

    /*****      Convert to grayscale     *******/
    Mat src_gray, dst_gray;

    rgb2g(src, dst_gray);                       // ours
    cvtColor(src, src_gray, CV_BGR2GRAY, 0);    // OpencV

    // Compare
    unsigned int diff = sumOfAbsoluteDifferences(src_gray, dst_gray);
    DLOG("gray abs diff %u", diff);

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
    DLOG("filter abs diff %u", diff);

    /*****      Isolate objects     *******/
    Mat src_obj;    // grayscale and binarize output from edge detection
    Mat dst_obj;    // for debug, draw bounding boxes around objects
    Mat obj[10];    // put each object in its own Mat

    cvtColor(dst_filter, src_obj, CV_BGR2GRAY, 0);
    threshold(src_obj, src_obj, 50, 255, THRESH_BINARY);

    dst_obj = src_obj.clone();
    Mat tmp = src_obj.clone();

    for (int i = 0; i < 10; i++) {
        struct rect r = extractObject(src_obj, dst_obj);
        obj[i] = tmp(Range(r.top, r.bottom), Range(r.left, r.right));
        //DLOG("obj %d is %d x %d", i, obj[i].cols, obj[i].rows);
    }

    /*****      Isolate color     *******/
    Mat red, bw, thresh;

    isolateColor(src, RED, red);
    cvtColor(red, bw, CV_BGR2GRAY, 0);
    //threshold(bw, thresh, 100, 255, THRESH_BINARY);
    //_moment color_moment = imageMoments(thresh);
    DLOG("");
    Moments color_moment = moments(bw, false);
    DLOG("");

    int xbar = (int)color_moment.m10 / color_moment.m00;
    int ybar = (int)color_moment.m01 / color_moment.m00;

    red.data[ybar * 3 * red.cols + xbar * 3 + BLUE] = 255;

    DLOG("%10s %12d", "xbar", xbar);
    DLOG("%10s %12d", "ybar", ybar);

    displayImageRow("Extract red", 2, &bw, &red);

    return 0;;
    /*****      Image moments     *******/
    for (int i = 0; i < 1; i++) {
        // Ours
        _moment _m = imageMoments(obj[i]);
        double *_hu = (double *)&_m.hu;

        // OpenCV
        double hu[7];
        Moments m = moments(obj[i], false);
        HuMoments(m, hu);

        DLOG("Image moments");
        DLOG("%10s %12s %12s", "", "OpenCV", "custom");
        DLOG("%10s %12.0f %12.0f", "m00", m.m00, _m.m00);
        DLOG("%10s %12.3f %12.3f", "xbar", m.m01 / m.m00, _m.m01 / _m.m00);
        DLOG("%10s %12.3f %12.3f", "ybar", m.m10 / m.m00, _m.m10 / _m.m00);
        DLOG("");

        /*
        DLOG("normalized central moments");
        DLOG("%10s %12.8f %12.8f", "n02", m.nu02, _m.n02);
        DLOG("%10s %12.8f %12.8f", "n03", m.nu03, _m.n03);
        DLOG("%10s %12.8f %12.8f", "n11", m.nu11, _m.n11);
        DLOG("%10s %12.8f %12.8f", "n12", m.nu12, _m.n12);
        DLOG("%10s %12.8f %12.8f", "n21", m.nu21, _m.n21);
        DLOG("%10s %12.8f %12.8f", "n20", m.nu20, _m.n20);
        DLOG("%10s %12.8f %12.8f", "n30", m.nu30, _m.n30);
        DLOG("");

        DLOG("Hu moments");
        DLOG("%10s %12s %12s", "", "OpenCV", "custom");
        for (int i = 0; i < 7; i++) {
            DLOG("hu[%d] %12.11f %12.11f", i, hu[i], _hu[i]);
        }
        DLOG("");
        */
    }

    /*
    DLOG("Image moments");
    DLOG("%10s %12s %12s", "", "OpenCV", "custom");
    DLOG("%10s %12.0f %12.0f", "m00", m.m00, _m.m00);
    DLOG("%10s %12.3f %12.3f", "xbar", m.m01 / m.m00, _m.m01 / _m.m00);
    DLOG("%10s %12.3f %12.3f", "ybar", m.m10 / m.m00, _m.m10 / _m.m00);
    DLOG("");
    DLOG("%10s %12e %12e", "u02", m.mu02, _m.u02);
    DLOG("%10s %12e %12e", "u03", m.mu03, _m.u03);
    DLOG("%10s %12e %12e", "u11", m.mu11, _m.u11);
    DLOG("%10s %12e %12e", "u12", m.mu12, _m.u12);
    DLOG("%10s %12e %12e", "u21", m.mu21, _m.u21);
    DLOG("%10s %12e %12e", "u20", m.mu20, _m.u20);
    DLOG("%10s %12e %12e", "u30", m.mu30, _m.u30);
    DLOG("");
    DLOG("%10s %12.8f %12.8f", "u02", m.nu02, _m.n02);
    DLOG("%10s %12.8f %12.8f", "u03", m.nu03, _m.n03);
    DLOG("%10s %12.8f %12.8f", "u11", m.nu11, _m.n11);
    DLOG("%10s %12.8f %12.8f", "u12", m.nu12, _m.n12);
    DLOG("%10s %12.8f %12.8f", "u21", m.nu21, _m.n21);
    DLOG("%10s %12.8f %12.8f", "u20", m.nu20, _m.n20);
    DLOG("%10s %12.8f %12.8f", "u30", m.nu30, _m.n30);
    DLOG("");
    DLOG("%10s %12.8f %12.8f", "hu[0]", hu[0], _hu[0]);
    DLOG("%10s %12.8f %12.8f", "hu[1]", hu[1], _hu[1]);
    DLOG("%10s %12.8f %12.8f", "hu[2]", hu[2], _hu[2]);
    DLOG("%10s %12.8f %12.8f", "hu[3]", hu[3], _hu[3]);
    DLOG("%10s %12.8f %12.8f", "hu[4]", hu[4], _hu[4]);
    DLOG("%10s %12.8f %12.8f", "hu[5]", hu[5], _hu[5]);
    DLOG("%10s %12.8f %12.8f", "hu[6]", hu[6], _hu[6]);
    */



    // Display results
    displayImageRow("Extract obj", 4, &src, &obj[0], &obj[1], &obj[2]);
    //displayImageRow("Source", src, src);
    //displayImageRow("Gray", src_gray, dst_gray);
    //displayImageRow("Filter", src, dst_filter);

    return 0;
}
