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
#include <opencv2/core.hpp>

#include "debug.h"
#include "img_proc.h"
#include "kernel.h"
#include "utils.h"

/**
 * Trackbar callback.  Invoked when value of trackbar is changed.
 *
 * Use the trackbar value to specify threshold to `isolateColor` (i.e. how red
 * does this pixel have to be to count as a RED pixel.
 *
 * @param x     Value of trackbar
 * @param data  Void pointer to source image.
 */
void locate_point_cb(int x, void *data)
{
    Mat red, bw, src;

    src = *(Mat *)data;

    isolateColor(src, RED, red, x);
    cvtColor(red, bw, CV_BGR2GRAY, 0);
    _moment color_moment = imageMoments(bw);

    // Avoid a divide-by-zero in the case that there are no red pixels.
    if (color_moment.m00 == 0) {
        WLOG("Unable to find centroid.");
        return;
    }

    int xbar = (int)color_moment.m10 / color_moment.m00;
    int ybar = (int)color_moment.m01 / color_moment.m00;

    // Draw a small blue circle at the centroid to visually identify it.
    circle(red, Point(xbar, ybar), 3, Scalar(255, 0, 0), -1);

    DLOG("Threshold %d\t Centroid (%d, %d)", x, xbar, ybar);

    imshow("Extract red 0", red);
}

using namespace cv;

/*****      Isolate color     *******/
void isolate_color(const Mat &src)
{
    int x;

    displayImageRow("Extract red", 1, &src);
    createTrackbar("Trackbar", "Extract red 0", &x, 255, locate_point_cb,
            (void *)&src);

    waitKey(0);
}

/*****      Convert to grayscale     *******/
void convert_to_grayscale(const Mat &src, Mat &dst)
{
    Mat dst_opencv;

    rgb2g(src, dst);                       // ours
    cvtColor(src, dst_opencv, CV_BGR2GRAY, 0);    // OpencV

    // Compare
    unsigned int diff = sumOfAbsoluteDifferences(dst_opencv, dst);
    DLOG("gray abs diff %u", diff);
    displayImageRow("Color to gray", 2, &dst_opencv, &dst);

}

/*****      Sobel edge detection     *******/
/**
 * \p src should be a grayscale image
 */
void sobel(const Mat &src, Mat &dst)
{
    Mat dst_opencv;
    unsigned int diff;

    // Ours
    Mat dst_x, dst_y;
    applyKernel(src, dst_x, kern_sobel_x);
    applyKernel(src, dst_y, kern_sobel_y);
    combine(dst_x, dst_y, dst, &hypoteneuse);

    // OpenCV
    Mat tmp_x, tmp_y;
    filter2D(src, tmp_x, CV_16S, kern_sobel_x);     // x derivative
    filter2D(src, tmp_y, CV_16S, kern_sobel_y);     // y derivative
    addWeighted(tmp_x, 0.5, tmp_y, 0.5, 0, dst_opencv); // combine
    convertScaleAbs(dst_opencv, dst_opencv);        // back to CV_8U

    // Compare
    diff = sumOfAbsoluteDifferences(dst_opencv, dst);
    DLOG("filter abs diff %u", diff);
    displayImageRow("Sobel", 2, &dst_opencv, &dst);

}

/*****      Isolate objects     *******/
/**
 * Draw rectangles onto \p dst representing bounding boxes.
 * Return each isolated but unidentified object in an array of objs.
 */
int isolate_objects(const Mat &src, Mat &dst, Mat obj[99])
{
    dst = src.clone();
    Mat tmp = src.clone();

    int num_objs = 0;
    int i = 0;
    struct rect r;
    do {
        r = extractObject(tmp, dst);        // extractObject  modifies tmp
        obj[i] = src(Range(r.top, r.bottom), Range(r.left, r.right));
        i++;
    } while(r.top != r.bottom && r.left != r.bottom);

    DLOG("Found %d objects.", i);
    num_objs = i;

    displayImageRow("obj", 5, &obj[0], &obj[1], &obj[2], &obj[3], &obj[4]);
    displayImageRow("annotated src", 1, &dst);

    return num_objs;
}

/*****      Image moments     *******/
/**
 * Array of objects in obj, number of object num_objs.
 * Annotate source with calculated moment invariants over each object.
 */
void moment_invariants(Mat &src, Mat obj[99], int num_objs)
{
    double *hu_g = (double *)malloc(sizeof(double) * 7 * num_objs);
    for (int i = 0; i < num_objs; i++) {
        // Ours
        _moment _m = imageMoments(obj[i]);

        if (_m.m00 == 0) break;

        // OpenCV
        double hu[7];
        Moments m = moments(obj[i], false);
        HuMoments(m, hu);

        for (int j = 0; j < 7; j++) {
            hu_g[i * 7 + j] = hu[j];
        }

        /*
        double *_hu = (double *)&_m.hu;
        DLOG("Image moments");
        DLOG("%10s %12s %12s", "", "OpenCV", "custom");
        DLOG("%10s %12.0f %12.0f", "m00", m.m00, _m.m00);
        DLOG("%10s %12.3f %12.3f", "xbar", m.m01 / m.m00, _m.m01 / _m.m00);
        DLOG("%10s %12.3f %12.3f", "ybar", m.m10 / m.m00, _m.m10 / _m.m00);
        DLOG("");

        DLOG("central moments");
        DLOG("%10s %12e %12e", "u02", m.mu02, _m.u02);
        DLOG("%10s %12e %12e", "u03", m.mu03, _m.u03);
        DLOG("%10s %12e %12e", "u11", m.mu11, _m.u11);
        DLOG("%10s %12e %12e", "u12", m.mu12, _m.u12);
        DLOG("%10s %12e %12e", "u21", m.mu21, _m.u21);
        DLOG("%10s %12e %12e", "u20", m.mu20, _m.u20);
        DLOG("%10s %12e %12e", "u30", m.mu30, _m.u30);
        DLOG("");
        */

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
            DLOG("hu[%d] %+33.32f %+33.32f", i, hu[i], _hu[i]);
        }
        DLOG("");
        */

        unsigned int r = compareHu(&hu_g[0], &hu_g[7 * i]);

        // For debug, write the calculated difference onto the source image.
        // Locate obj within src image.
        Point ofs;
        Size parent_size;
        obj[i].locateROI(parent_size, ofs);

        // Write the number to the image, at the object.
        char buf[256];
        sprintf(buf, "%d", r);
        putText(src, buf, ofs, FONT_HERSHEY_PLAIN, 1, Scalar::all(255), 1);
    }

    displayImageRow("Hu moments", 1, &src);
}

/**
 * Find connected components in source image.  Re-draw the source image with
 * each label painted a different color in dst.
 *
 * \p src should be a binary image.
 */
void connected_components(const Mat &src, Mat &dst)
{
    Mat m_labels;
    int labels = connectedComponents(src, m_labels);

    // Connected components labels them 1, 2, 3, ...
    // which basically looks like black.
    //
    // Re-label them in different colors.
    std::vector<Vec3b> colors(labels);
    colors[0] = Vec3b(0, 0, 0); //background
    for(int i = 1; i < labels; i++){
        colors[i] = Vec3b( (rand()&255), (rand()&255), (rand()&255) );
    }

    dst = Mat(src.size(), CV_8UC3);
    for(int r = 0; r < dst.rows; ++r){
        for(int c = 0; c < dst.cols; ++c){
            int label = m_labels.at<int>(r, c);
            Vec3b &pixel = dst.at<Vec3b>(r, c);
            pixel = colors[label];
         }
     }

    DLOG("found %d labels", labels);
    displayImageRow("connected components", 2, &src, &dst);
}


int main(int argc, char** argv )
{
    Mat src;                    // Load source image.
    Mat dst;
    Mat objs[99];

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


    // Parse args and perform functions as requested.
    char buf[256];
    for (;;) {
        printf("Enter command:\n");
        scanf("%256s", buf);

        if (buf[0] == 'c') {
            Mat m_gray, m_sobel, m_thresh;

            convert_to_grayscale(src, m_gray);
            resetDisplayPosition();

            sobel(m_gray, m_sobel);
            threshold(m_sobel, m_thresh, 150, 255, THRESH_BINARY);
            resetDisplayPosition();

            connected_components(m_thresh, dst);
        }
        else if (buf[0] == 'i') {
            isolate_color(src);
        }
        else if (buf[0] == 'g') {
            convert_to_grayscale(src, dst);
        }
        else if (buf[0] == 'm') {
            Mat m_gray, m_sobel, m_thresh;

            convert_to_grayscale(src, m_gray);
            resetDisplayPosition();

            sobel(m_gray, m_sobel);
            threshold(m_sobel, m_thresh, 150, 255, THRESH_BINARY);
            resetDisplayPosition();

            int num_objs = isolate_objects(m_thresh, dst, objs);
            resetDisplayPosition();

            moment_invariants(src, objs, num_objs);
        }
        else if (buf[0] == 'o') {
            Mat m_gray, m_sobel, m_thresh;

            convert_to_grayscale(src, m_gray);
            resetDisplayPosition();

            sobel(m_gray, m_sobel);
            threshold(m_sobel, m_thresh, 150, 255, THRESH_BINARY);
            resetDisplayPosition();

            isolate_objects(m_thresh, dst, objs);
        }
        else if (buf[0] == 's') {
            Mat m_gray;

            convert_to_grayscale(src, m_gray);
            resetDisplayPosition();

            sobel(m_gray, dst);
        }
        else if (buf[0] == 'q') {
            break;
        }
        else {
                DLOG("Usage:");
                DLOG("    c: Find connected components.");
                DLOG("    i: Isolate color, with threshold trackbar.");
                DLOG("    g: Convert color to grayscale.");
                DLOG("    m: Calculate moment invariants.  Annotates source.");
                DLOG("    o: Isolate objects.  Draws bounding boxes.");
                DLOG("    s: Apply Sobel operator.");
        }
        resetDisplayPosition();
    }

    return 0;
}
