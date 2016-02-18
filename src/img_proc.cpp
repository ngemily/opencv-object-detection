/**
 * Image processing functions written to implement OpenCV functionality in C.
 *
 * @file img_proc.cpp
 * @author Emily Ng
 * @date Feb 11 2016
 */

#include <assert.h>
#include <math.h>
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
            dst.data[i * cols + j] = saturate_cast<uchar>(
                B_WEIGHT * src.data[src_idx]
                + G_WEIGHT * src.data[src_idx + 1]
                + R_WEIGHT * src.data[src_idx + 2]);
        }
    }

}

/** Apply a kernel to source image.
 *
 * @param src       source image
 * @param dst       dest image
 * @param kernel    kernel
 */
void applyKernel(Mat &src, Mat &dst, const Mat &kernel)
{
    DLOG("kernel %d x %d\n", kernel.size().width, kernel.size().height);
    DLOG("src    %d x %d\n", src.size().width, src.size().height);
    DLOG("dst    %d x %d\n", dst.size().width, dst.size().height);

    const int rows = src.size().height;
    const int cols = src.size().width;
    const int num_channels = src.channels();

    assert(kernel.isContinuous());
    assert(src.isContinuous());

    dst = Mat::zeros(rows, cols, src.type());

    assert(dst.isContinuous());

    int i, j;

    DLOG("Kernel:\n");
    const char *k = (char *)kernel.data;
    for (i = 0; i < kernel.rows * kernel.cols; i++) {
        printf("%4d ", k[i]);
        if (i % 3 == 2)
            printf("\n");
    }

    const int len_row = cols * num_channels;
    for (i = 1; i < rows - 1; i++) {
        for (j = num_channels; j < num_channels * (cols - 1); j++) {
            // compute convolution for this pixel
            //
            // assume 3x3 kernel for now

            // OpenCV seems to reverse mapping between kernel and image.
            const uchar *p = &src.data[i * len_row + j];
            int pixel =
                  k[0] * p[-num_channels - len_row]
                + k[1] * p[0 - len_row]
                + k[2] * p[+num_channels - len_row]

                + k[3] * p[-num_channels]
                + k[4] * p[0]
                + k[5] * p[+num_channels]

                + k[6] * p[-num_channels + len_row]
                + k[7] * p[0 + len_row]
                + k[8] * p[+num_channels + len_row]
                ;
            dst.data[i * len_row + j] = saturate_cast<uchar>(abs(pixel));
        }
    }
}

/** Combine two images into a third, by a given function.
 *
 * Each pixel in C is calculated as a function of the corresponding pixel in A
 * and in B.  The combining function should accept two numbers and return a
 * number.

 * @param A     Source image.
 * @param B     Source image.
 * @param C     Dest image.
 * @param func  Pointer to combining function.
 */
void combine(Mat &A, Mat &B, Mat &C, int (*fp)(int a, int b))
{
    assert(A.depth() == B.depth());
    assert(A.channels() == B.channels());
    assert(A.rows == B.rows && A.cols == B.cols);

    assert(A.isContinuous());
    assert(B.isContinuous());

    const int rows = A.rows;
    const int cols = A.cols;
    const int num_channels = A.channels();

    C = Mat::zeros(rows, cols, A.type());

    assert(C.isContinuous());

    int i, j, idx=0;;
    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols * num_channels; j++) {
            uchar a = A.data[idx];
            uchar b = B.data[idx];
            int p = fp(a, b);

            C.data[idx] = saturate_cast<uchar>(p);
            idx++;
        }
    }
}

/**
 * Extract a single contour from an image of many contours.
 *
 * Modifies src by erasing the image that it finds.  Modifies dst by drawing
 * lines that go out from the start pixel to the top-left and bottom-right
 * corners.
 *
 * Only identifies connected contours.  Contour may have holes in it.
 *
 * @param src   Binary grayscale image of contours
 * @param dst   Bounding corners drawn.
 */
struct rect extractObject(Mat &src, Mat &dst)
{
    const int rows = src.rows;
    const int cols = src.cols;

    assert(src.channels() == 1);

    assert(src.isContinuous());
    assert(dst.isContinuous());

    DLOG("%s\n", __FUNCTION__);
    DLOG("src    %d x %d\n", src.size().width, src.size().height);

    int start_x, start_y;
    int top, left, bottom, right;

    struct rect r = (struct rect) {0, 0, 0, 0};

    // Find first pixel
    int i=0, j=0, idx=0;;
    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            if (src.data[idx] == WHITE) {
                dst.data[idx] = WHITE;
                start_x = j;
                start_y = i;
                DLOG("start pixel (%d, %d)\n", i, j);
                goto find_bottom_right;
            }

            idx++;
        }
    }

    // Did not find any white pixels.
    DLOG("empty image\n");
    return r;

    // Inch forwards diagonally until no more pixels
find_bottom_right:
    j+=10; i+=10;
    for (;;) {
        // if (no white pixels in row i or col j) return;

        int found_pixel = 0;
        // check col j
        for (int ii = 0; ii < i; ii++) {
            if (src.data[ii * cols + j] == WHITE) {
                dst.data[i * cols + j] = WHITE;
                found_pixel = 1;
                j++;
                break;
            }
        }

        // check row i
        for (int jj = 0; jj < j; jj++) {
            if (src.data[i * cols + jj] == WHITE) {
                dst.data[i * cols + j] = WHITE;
                found_pixel = 1;
                i++;
                break;
            }
        }

        if (!found_pixel || i == rows || j == cols) {
            dst.data[i * cols + j] = WHITE;
            DLOG("bottom right (%d, %d)\n", i, j);
            bottom = i;
            right = j;
            goto find_top_left;
        }

    }

find_top_left:
    j = start_x;
    i = start_y;

    for (;;) {
        // if (no white pixels in row i or col j) return;

        int found_pixel = 0;
        // check col j
        for (int ii = bottom; ii >= i; ii--) {
            if (src.data[ii * cols + j] == WHITE) {
                dst.data[i * cols + j] = WHITE;
                found_pixel = 1;
                j--;
                break;
            }
        }

        // check row i
        for (int jj = right; jj >= j; jj--) {
            if (src.data[i * cols + jj] == WHITE) {
                dst.data[i * cols + j] = WHITE;
                found_pixel = 1;
                i--;
                break;
            }
        }

        if (i == 0 || j == 0) {
            DLOG("out of image\n");
        }
        if (!found_pixel || i == 0 || j == 0) {
            dst.data[i * cols + j] = WHITE;
            DLOG("top left (%d, %d)\n", i, j);
            top = i;
            left = j;
            goto end;
        }

    }


end:
    // Erase from src image.
    for (int ii = top; ii < bottom; ii++) {
        for (int jj = left; jj < right; jj++) {
            idx = ii * cols + jj;
            src.data[idx] = BLACK;
        }
    }

    r.top = top;
    r.bottom = bottom;
    r.left = left;
    r.right = right;

    DLOG("obj is %d x %d\n", r.right - r.left, r.bottom - r.top);

    return r;
}

struct _moment imageMoments(Mat &src)
{
    DLOG("src    %d x %d\n", src.size().width, src.size().height);

    const int rows = src.rows;
    const int cols = src.cols;

    struct _moment m;

    m.m00 = 0;
    m.m01 = 0;
    m.m10 = 0;
    m.u02 = 0;
    m.u03 = 0;
    m.u11 = 0;
    m.u12 = 0;
    m.u21 = 0;
    m.u02 = 0;
    m.u03 = 0;

    int i, j, idx=0;

    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            m.m00 += src.data[idx];
            m.m01 += i * src.data[idx];
            m.m10 += j * src.data[idx];
            idx++;
        }
    }

    const float x_bar = m.m10 / m.m00;
    const float y_bar = m.m01 / m.m00;

    idx=0;
    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            const float x_dist = j - x_bar;
            const float y_dist = i - y_bar;

            m.u02 += pow(y_dist, 2) * src.data[idx];
            m.u03 += pow(y_dist, 3) * src.data[idx];
            m.u11 += x_dist * y_dist * src.data[idx];
            m.u12 += x_dist * pow(y_dist, 2) * src.data[idx];
            m.u21 += pow(x_dist, 2) * y_dist * src.data[idx];
            m.u20 += pow(x_dist, 2) * src.data[idx];
            m.u30 += pow(x_dist, 3) * src.data[idx];

            idx++;
        }
    }

    // n_ij = u_ij / (m_00 ^ (1 + (i + j) / 2))
    m.n02 = m.u02 / pow(m.m00, 1 + (0 + 2) / 2.0);
    m.n03 = m.u03 / pow(m.m00, 1 + (0 + 3) / 2.0);
    m.n11 = m.u11 / pow(m.m00, 1 + (1 + 1) / 2.0);
    m.n12 = m.u12 / pow(m.m00, 1 + (1 + 2) / 2.0);
    m.n20 = m.u20 / pow(m.m00, 1 + (2 + 0) / 2.0);
    m.n21 = m.u21 / pow(m.m00, 1 + (2 + 1) / 2.0);
    m.n30 = m.u30 / pow(m.m00, 1 + (3 + 0) / 2.0);

    m.hu[0] = m.n20 + m.n02;
    m.hu[1] = pow(m.n20 - m.n02, 2) + 4 * pow(m.n11, 2);
    m.hu[2] = pow(m.n30 - 3 * m.n12, 2) + pow(3 * m.n21 - m.n03, 2);
    m.hu[3] = pow(m.n30 + m.n12, 2) + pow(m.n21 + m.n03, 2);
    m.hu[4] = (m.n30 - 3 * m.n12) * (m.n30 + m.n12)
        * (pow(m.n30 + m.n12, 2) - 3 * pow(m.n21 + m.n03, 2))
        + (3 * m.n21 - m.n03) * (m.n21 + m.n03)
        * (3 * pow(m.n30 - m.n12, 2) - pow(m.n21 + m.n03, 2));
    m.hu[5] = (m.n20 - m.n02)
        * (pow(m.n30 + m.n12, 2) - pow(m.n21 + m.n03, 2))
        + 4 * m.n11 * (m.n30 + m.n12) * (m.n21 + m.n03);
    m.hu[6] = (3 * m.n21 - m.n03) * (m.n30 + m.n12)
        * (pow(m.n30 + m.n12, 2) - 3 * pow(m.n21 + m.n03, 2))
        - (m.n30 - 3 * m.n12) * (m.n21 + m.n03)
        * (3 * pow(m.n30 + m.n12, 2) - pow(m.n21 + m.n03, 2));

    return m;
}
