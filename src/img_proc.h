/**
 * Image processing functions written to implement OpenCV functionality in C.
 *
 * @file img_proc.h
 * @author Emily Ng
 * @date Feb 11 2016
 */

#ifndef __IMG_PROC_H
#define __IMG_PROC_H

#include <stdio.h>
#include <stdlib.h>
#include <opencv/cv.h>

#include "debug.h"

// weights for RGB to grayscale conversion
#define R_WEIGHT (0.2990)
#define G_WEIGHT (0.5870)
#define B_WEIGHT (0.1140)

// number of channels in grayscale or color image
#define COLOR 3
#define GRAY 1

// channel offset for BGR image
#define BLUE (0)
#define GREEN (1)
#define RED (2)

// saturate values for 8-bit grayscale image
#define WHITE (255)
#define BLACK (0)

using namespace cv;

struct _moment{
    // moment about 0
    double m00;
    double m10;
    double m01;

    // central moment
    double u02;
    double u03;
    double u11;
    double u12;
    double u20;
    double u21;
    double u30;

    // normalized moment
    double n02;
    double n03;
    double n11;
    double n12;
    double n20;
    double n21;
    double n30;

    // Hu moment
    double hu[7];
};

struct rect {
    int top;
    int bottom;
    int left;
    int right;
};

unsigned int sumOfAbsoluteDifferences(Mat &A, Mat &B);
void rgb2g(const Mat &src, Mat &dst);
void applyKernel(const Mat &src, Mat &dst, const Mat &kernel);
void combine(Mat &A, Mat &B, Mat &C, int (*fp)(int a, int b));
struct rect extractObject(Mat &src, Mat &dst);
struct _moment imageMoments(const Mat &src);
void isolateColor(const Mat &src, const int c, Mat &dst, uchar thresh);
unsigned int compareHu(double *hu1, double *hu2);

#endif
