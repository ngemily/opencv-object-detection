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

// saturate values for 8-bit grayscale image
#define WHITE (255)
#define BLACK (0)

using namespace cv;

struct _moment{
    // moment about 0
    float m00;
    float m10;
    float m01;

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

unsigned int sumOfAbsoluteDifferences(Mat &A, Mat &B);
void rgb2g(Mat &src, Mat &dst);
void applyKernel(Mat &src, Mat &dst, const Mat &kernel);
void combine(Mat &A, Mat &B, Mat &C, int (*fp)(int a, int b));
void extractObject(Mat &src, Mat &dst);
struct _moment imageMoments(Mat &src);

#endif
