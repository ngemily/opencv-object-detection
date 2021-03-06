/**
 * Image processing functions written to implement OpenCV functionality in C.
 *
 * @file img_proc.cpp
 * @author Emily Ng
 * @date Feb 11 2016
 */

#include <assert.h>
#include <math.h>
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <opencv2/imgproc.hpp>

#include "img_proc.h"

/** Compute sum of absolute value of differences of each pixel in two images.
 *
 * Images A and B must be of the same size, same depth, same number of channels.
 * Slight hack, ignore border pixels for purposes of comparing filtered images
 * with undefined behaviour at borders.
 *
 * @param A     reference to one image
 * @param B     reference to another image
 *
 * @return      Sum of pixel-wise absolute difference between \p A and \p B.
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

    ILOG("absdiff %f", sum / (float) (rows * cols * num_channels));
    return sum;
}

/** @brief Convert color image to gray image.
 *
 * @param src   source image
 * @param dst   dest image
 */
void rgb2g(const Mat &src, Mat &dst)
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
void applyKernel(const Mat &src, Mat &dst, const Mat &kernel)
{
    ILOG("kernel %d x %d", kernel.size().width, kernel.size().height);
    ILOG("src    %d x %d", src.size().width, src.size().height);
    ILOG("dst    %d x %d", dst.size().width, dst.size().height);

    const int rows = src.size().height;
    const int cols = src.size().width;
    const int num_channels = src.channels();

    assert(kernel.isContinuous());
    assert(src.isContinuous());

    dst = Mat::zeros(rows, cols, src.type());

    assert(dst.isContinuous());

    int i, j;

    ILOG("Kernel:");
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
 *
 * @return A rect struct that defines the boundaries of the identified object.
 */
struct rect extractObject(Mat &src, Mat &dst)
{
    const int rows = src.rows;
    const int cols = src.cols;

    assert(src.channels() == GRAY);

    assert(src.isContinuous());
    assert(dst.isContinuous());

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
                goto find_bottom_right;
            }

            idx++;
        }
    }

    // Did not find any white pixels.
    ILOG("empty image");
    return r;

    // Inch forwards diagonally until no more pixels
find_bottom_right:
    j+=10; i+=10;
    for (;;) {
        // if (no white pixels in row i or col j) return;

        int found_pixel = 0;
        // check col j
        for (int ii = start_y; ii < i; ii++) {
            if (src.data[ii * cols + j] == WHITE) {
                dst.data[i * cols + j] = WHITE;
                found_pixel = 1;
                j++;
                break;
            }
        }

        // check row i
        for (int jj = start_x; jj < j; jj++) {
            if (src.data[i * cols + jj] == WHITE) {
                dst.data[i * cols + j] = WHITE;
                found_pixel = 1;
                i++;
                break;
            }
        }

        if (i == rows || j == cols) {
            WLOG("at bottom right corner of image");
        }
        if (!found_pixel || i == rows || j == cols) {
            dst.data[i * cols + j] = WHITE;
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
            WLOG("at top left corner of image");
        }
        if (!found_pixel || i == 0 || j == 0) {
            dst.data[i * cols + j] = WHITE;
            top = i;
            left = j;
            goto end;
        }

    }


end:
    // Erase from src image.
    for (int i = top; i < bottom; i++) {
        for (int j = left; j < right; j++) {
            idx = i * cols + j;
            src.data[idx] = BLACK;
        }
    }

    r.top = top;
    r.bottom = bottom;
    r.left = left;
    r.right = right;

    // draw bounding box
    rectangle(dst, Point(left, top), Point(right, bottom), Scalar::all(255));
    ILOG("obj is %d x %d", r.right - r.left, r.bottom - r.top);

    return r;
}

/**
 * Calculate moments of an image.
 *
 * Calculate moments about zero, about centroid, normalized moments about
 * centroid, and Hu's moment invariants of an image.
 *
 * @param src   Source image.
 */
struct _moment imageMoments(const Mat &src)
{
    assert(src.channels() == GRAY);

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
    m.u20 = 0;
    m.u30 = 0;

    int i, j;
    const uchar *p;

    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            p = src.ptr<uchar>(i);

            m.m00 += p[j];
            m.m01 += i * p[j];
            m.m10 += j * p[j];
        }
    }

    if (m.m00 == 0) {
        WLOG("m.m00 == 0");
        return m;
    }

    const double x_bar = m.m10 / m.m00;
    const double y_bar = m.m01 / m.m00;

    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            const double x_dist = j - x_bar;
            const double y_dist = i - y_bar;

            // u_ij = (x - x_bar) ^ i * (y - y_bar) ^ j * src[i, j]
            p = src.ptr<uchar>(i);
            m.u02 += pow(y_dist, 2) * p[j];
            m.u03 += pow(y_dist, 3) * p[j];
            m.u11 += x_dist * y_dist * p[j];
            m.u12 += x_dist * pow(y_dist, 2) * p[j];
            m.u21 += pow(x_dist, 2) * y_dist * p[j];
            m.u20 += pow(x_dist, 2) * p[j];
            m.u30 += pow(x_dist, 3) * p[j];
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
        * (3 * pow(m.n30 + m.n12, 2) - pow(m.n21 + m.n03, 2));
    m.hu[5] = (m.n20 - m.n02)
        * (pow(m.n30 + m.n12, 2) - pow(m.n21 + m.n03, 2))
        + 4 * m.n11 * (m.n30 + m.n12) * (m.n21 + m.n03);
    m.hu[6] = (3 * m.n21 - m.n03) * (m.n30 + m.n12)
        * (pow(m.n30 + m.n12, 2) - 3 * pow(m.n21 + m.n03, 2))
        - (m.n30 - 3 * m.n12) * (m.n21 + m.n03)
        * (3 * pow(m.n30 + m.n12, 2) - pow(m.n21 + m.n03, 2));

    return m;
}

/**
 * Isolate a single color from image.
 *
 * Simply extracting a single color isn't very helpful, is there is a strong
 * intensity of each color in white.  Subtract the common value between each
 * channel, and apply a threshold for desired channel over the other channels.
 *
 * NB: What if a pixel has the value (240, 0, 255)?  This method doesn't take
 * into account all three channels very well.
 *
 * @param src       3 channel (color) image
 * @param channel   Color to be isolated
 * @param dst       Copy of @src with only desired color.
 * @param thresh    Threshold for determining that a pixel is a certain color.
 */
void isolateColor(const Mat &src, const int c, Mat &dst, uchar thresh)
{
    assert(src.isContinuous());
    assert(src.channels() == COLOR);

    dst = Mat::zeros(src.size(), src.type());

    const int rows = src.rows;
    const int cols = src.cols;
    const int num_channels = src.channels();

    ILOG("isolating channel %d", c);

    int i, j, idx=0;
    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols * num_channels; j+=num_channels) {
            idx+=num_channels;

            uchar r = src.data[idx + RED];
            uchar g = src.data[idx + GREEN];
            uchar b = src.data[idx + BLUE];

            uchar min = (r <= g && r <= b) ? r :
                        (g <= r && g <= b) ? g :
                        (b <= r && b <= g) ? b :
                        0;

            uchar p = src.data[idx + c] - min;

            if (p > thresh) {
                dst.data[idx + c] = p;
            }
        }
    }
}

/**
 * Compare two sets of Hu moments to see if we have a match.
 *
 * The two sets of moments are treated the same, i.e. the two arguments are
 * commutative.
 *
 * @param hu1   Reference image's moments.
 * @param hu2   Sample image's moments.
 *
 * @return A number representing how different the two images are.  A value < 50
 * is a pretty good match.
 */
unsigned int compareHu(double *hu1, double *hu2)
{
    double r = 0;

    // NB: The 7th moment is for skew invariance.  At the moment we do not want
    // to consider it as our shapes can become quite similar if stretched.
    for (int i = 0; i < 6; i++) {
        double h1 = (hu1[i]);
        double h2 = (hu2[i]);

        double sq_diff = pow(h2 - h1, 2) / (h1 * h2);

        r += pow(sq_diff, 2);
    }

    return (unsigned int) r;
}

/**
 * Connected components labeling.
 *
 * @param src   Source (binary) image.
 * @param dst   Each connected component replaced with label.
 * @return Number of components found.
 */
unsigned int connectedComponentsLabeling(const Mat &src, Mat &dst)
{
    assert(src.isContinuous());
    assert(src.channels() == GRAY);

    dst = Mat::zeros(src.size(), src.type());

    assert(dst.isContinuous());
    assert(dst.channels() == GRAY);

    const int rows = src.rows;
    const int cols = src.cols;

    int num_labels = 0;
    int merge_table[1024];


    int i, j, idx=cols;

    // Initialize merge table.
    merge_table[0] = 0;

    // First pass.
    for (i = 1; i < rows - 1; i++) {
        std::stack<struct merge_entry> merge_stack;
        for (j = 1; j < cols - 1; j++) {
            idx = i * cols + j;

            int a_idx = (i - 1) * cols + j - 1;
            int b_idx = (i - 1) * cols + j - 0;
            int c_idx = (i - 1) * cols + j + 1;
            int d_idx = i * cols + j -1;

            // 8-connected neighbours that have already been processed.
            uchar a = dst.data[a_idx];
            uchar b = dst.data[b_idx];
            uchar c = dst.data[c_idx];
            uchar d = dst.data[d_idx];

            // pixel to process
            uchar p = src.data[idx];

            // Determine label
            //
            // Background, no need to label.
            if (p == BLACK) {
                continue;
            }
            // Neighbours are background, make a new label.
            else if ((a | b | c | d) == BLACK) {
                num_labels++;
                dst.data[idx] = num_labels;
                merge_table[num_labels] = num_labels;
            }
            // Check for single label.
            else if ((a == b || b == BLACK)
                    && (a == c || c == BLACK)
                    && (a == d || d == BLACK)) {
                DLOG("%u %u %u %u : a -> %u", a, b, c, d, merge_table[a]);
                dst.data[idx] = merge_table[dst.data[a_idx]];
            }
            else if ((b == a || a == BLACK)
                    && (b == c || c == BLACK)
                    && (b == d || d == BLACK)) {
                DLOG("%u %u %u %u : b", a, b, c, d);
                DLOG("%u %u %u %u : c -> %u", a, b, c, d, merge_table[b]);
                dst.data[idx] = merge_table[dst.data[b_idx]];
            }
            else if ((c == b || b == BLACK)
                    && (c == a || a == BLACK)
                    && (c == d || d == BLACK)) {
                DLOG("%u %u %u %u : c -> %u", a, b, c, d, merge_table[c]);
                dst.data[idx] = merge_table[dst.data[c_idx]];
            }
            else if ((d == b || b == BLACK)
                    && (d == c || c == BLACK)
                    && (d == a || a == BLACK)) {
                DLOG("%u %u %u %u : d -> %u", a, b, c, d, merge_table[d]);
                dst.data[idx] = merge_table[dst.data[d_idx]];
            }
            // Two or more labels have been found.  Need to merge.
            // First update merge table.
            else {
                uchar min = -1;         // target

                min = (a != 0 && a < min) ? a : min;
                min = (b != 0 && b < min) ? b : min;
                min = (c != 0 && c < min) ? c : min;
                min = (d != 0 && d < min) ? d : min;

                if (a != 0 && a != min) {
                    dst.data[a_idx] = min;
                    struct merge_entry entry = {.index = a, .target = min};
                    merge_stack.push(entry);
                }
                if (b != 0 && b != min) {
                    dst.data[b_idx] = min;
                    struct merge_entry entry = {.index = b, .target = min};
                    merge_stack.push(entry);
                }
                if (c != 0 && c != min) {
                    dst.data[c_idx] = min;
                    struct merge_entry entry = {.index = c, .target = min};
                    merge_stack.push(entry);
                }
                if (d != 0 && d != min) {
                    dst.data[d_idx] = min;
                    struct merge_entry entry = {.index = d, .target = min};
                    merge_stack.push(entry);
                }

                dst.data[idx] = merge_table[min];
                DLOG("%u %u %u %u min: %u -> %u", a, b, c, d, min,
                        merge_table[min]);

            }
        }

        // Resolve merges for this row
        DLOG("row %u: merges %lu", i, merge_stack.size());
        while (!merge_stack.empty()) {
            struct merge_entry entry = merge_stack.top();
            int index = entry.index;
            int target = entry.target;

            DLOG("%u -> %u", index, target);
            merge_table[index] = merge_table[target];

            merge_stack.pop();
        }
    }

    // Run through again and update using merge table.
    for (i = 1; i < rows - 1; i++) {
        for (j = 1; j < cols - 1; j++) {
            idx = i * cols + j;
            uchar label = dst.data[idx];
            dst.data[idx] = merge_table[label];
        }
    }

    FILE *fp = fopen("merge_table.txt", "w");
    for (int i = 0; i <= num_labels; i++) {
        fprintf(fp, "%u -> %u\n", i, merge_table[i]);
    }
    fclose(fp);

    fp = fopen("labels.txt", "w");
    for (int i = 1; i < rows - 1; i++) {
        for (int j = 1; j < rows - 1; j++) {
            int idx = i * cols + j;
            fprintf(fp, "%3d, ", dst.data[idx]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);

    ILOG("Found %d labels", num_labels);

    return num_labels;
}
