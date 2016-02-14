/**
 * @file img_proc.c
 * @author Emily Ng
 * @date Feb 11 2016
 * @brief Image processing functions written to implement OpenCV functionality
 * in more C like programming.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "img_proc.h"

/** Compute sum of absolute value of differences of each pixel in two images.
 *
 * Images A and B must be of the same size, same depth, same number of channels.
 * Slight hack, ignore border pixels for purposes of comparing filtered images
 * with undefined behaviour at borders.
 *
 * @param A     reference to one image
 * @param B     reference to another image
 */
unsigned int sumOfAbsoluteDifferences(Mat &A, Mat &B)
{
    assert(A.depth() == B.depth());
    assert(A.channels() == B.channels());
    assert(A.rows == B.rows && A.cols == B.cols);

    unsigned int sum = 0;
    int i, j;

    int rows = A.rows;
    int cols = A.cols;
    int num_channels = A.channels();

    for (i = 1; i < rows - 1; i++) {
        for (j = num_channels; j < num_channels * (cols - 1); j++) {
            int d = A.data[i * cols * num_channels + j]
                - B.data[i * cols * num_channels + j];
            sum += abs(d);
        }
    }

    DLOG("absdiff %f\n", sum / (float) (rows * cols * num_channels));
    return sum;
}

/** @brief Convert color image to gray image.
 *
 * @param src   source image
 * @param dst   dest image
 */
void rgb2g(Mat &src, Mat &dst)
{
    // intensity = 0.2989*red + 0.5870*green + 0.1140*blue
    const int rows = src.rows;
    const int cols = src.cols;

    assert(3 == src.channels());
    assert(src.isContinuous());

    dst = Mat::zeros(rows, cols, CV_8U);

    assert(dst.isContinuous());

    int num_channels = src.channels();
    int i, j;

    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            int src_idx = i * cols * num_channels + j * num_channels;

            // NB: endianness causes RGB to be stored as BGR
            dst.data[i * cols + j] = B_WEIGHT * src.data[src_idx]
                + G_WEIGHT * src.data[src_idx + 1]
                + R_WEIGHT * src.data[src_idx + 2];
        }
    }

}

/** Apply a kernel to source image.
 *
 * @param src       source image
 * @param dst       dest image
 * @param kernel    kernel
 */
void applyKernel(InputArray src, OutputArray dst, InputArray kernel)
{
    DLOG("kernel %d x %d\n", kernel.size().width, kernel.size().height);
    DLOG("src    %d x %d\n", src.size().width, src.size().height);
    DLOG("dst    %d x %d\n", dst.size().width, dst.size().height);

    const int rows = src.size().height;
    const int cols = src.size().width;
    const int num_channels = src.channels();

    int i, j, k;
    for (int i = 1; i < rows - 1; i++) {
        for (int j = num_channels; j < num_channels * (cols - 1); j++) {
            // compute convolution for this pixel
        }
    }
}

