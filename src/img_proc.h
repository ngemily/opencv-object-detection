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

using namespace cv;

unsigned int sumOfAbsoluteDifferences(Mat &A, Mat &B);
void rgb2g(Mat &src, Mat &dst);
void applyKernel(Mat &src, Mat &dst, Mat &kernel);

#endif
