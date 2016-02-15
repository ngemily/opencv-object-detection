#ifndef __UTILS_H
#define __UTILS_H

#include <opencv2/opencv.hpp>

#define DISP 1          // Toggle display of images.
#define PADDING 20      // Padding between images.

using namespace cv;

static int x = 0;
static int y = 0;

void displayImagePair(const char *window_name, Mat &img1, Mat &img2);

#endif
