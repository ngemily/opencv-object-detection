/**
 * Define kernels for image filtering.
 *
 * @file kernel.h
 * @author Emily Ng
 * @date Feb 15 2016
 */

#ifndef __KERNEL_H
#define __KERNEL_H

#include <opencv2/opencv.hpp>

const cv::Mat kern_sharpen = (cv::Mat_<char>(3, 3) <<
     0, -1,  0,
    -1,  5, -1,
     0, -1,  0);

const cv::Mat kern_sobel_x = (cv::Mat_<char>(3, 3) <<
    -1,  0,  1,
    -2,  0,  2,
    -1,  0,  1);

const cv::Mat kern_sobel_y = (cv::Mat_<char>(3, 3) <<
    -1, -2, -1,
     0,  0,  0,
     1,  2,  1);

#endif
