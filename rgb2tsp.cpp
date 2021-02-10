//
// Background thread processing of RGB image into <vector> of points in TSP order
//
#include "imgui.h"
#include "rgb2tsp.hpp"
#include <chrono>

using namespace cv;
using namespace std::chrono;

void ColorQuantize(cv::Mat& image, int k)
{
    // https://answers.opencv.org/question/182006/opencv-c-k-means-color-clustering/
    // convert to float & reshape to a [3 x W*H] Mat 
    //  (so every pixel is on a row of it's own)
    Mat data;
    image.convertTo(data, CV_32F);
    data = data.reshape(1, data.total());

    // do kmeans
    Mat labels, centers;
    kmeans(data, k, labels, TermCriteria(TermCriteria::MAX_ITER, 10, 1.0), 3, KMEANS_PP_CENTERS, centers);

    // reshape both to a single row of Vec3f pixels:
    centers = centers.reshape(3, centers.rows);
    data = data.reshape(3, data.rows);

    // replace pixel values with their center value:
    Vec3f* p = data.ptr<Vec3f>();
    for (size_t i = 0; i < data.rows; i++) {
        int center_id = labels.at<int>(i);
        p[i] = centers.at<Vec3f>(center_id);
    }

    // back to 2d, and uchar:
    image = data.reshape(3, image.rows);
    image.convertTo(image, CV_8U);
}

Path mat_to_tsp(cv::Mat& image, const std::atomic_bool& cancelled)
{
    Path tsp;

    // image = ImageAdjust[image, {0,0.9}] - lighten the image to blow out the face highlights
    image.convertTo(image, -1, 2.0);
    if (cancelled)
        return tsp;

    // ColorConvert[image,"Grayscale"] - converts the color space of image to the specified color space colspace.
//    cvtColor(image, image, COLOR_BGR2GRAY);
    if (cancelled)
        return tsp;

    // image = ColorQuantize[image, 2] - reduce the number of colors used to represent the image to 2 with dithering
    ColorQuantize(image, 2);
    if (cancelled)
        return tsp;
//    debug_image = image;
/*
    image = ImageAdjust[image] (* push colors to 0 or 255 *)
    if (cancelled)
        return tsp;
    pos = PixelValuePositions[image,0]; (* collect positions of all black pixels *)
    if (cancelled)
        return tsp;
    res = FindShortestTour[pos]; (* Use TSP to find path between all black pixels *)
*/

// Make sure that function takes a few seconds to complete
    std::this_thread::sleep_for(seconds(5));
    if (cancelled)
        return tsp;

    // create some dummy points just to have something to draw
    tsp.push_back({ 10, 10 });
    tsp.push_back({ 20, 10 });
    tsp.push_back({ 20, 20 });
    tsp.push_back({ 30, 20 });
    tsp.push_back({ 30, 30 });
    tsp.push_back({ 40, 30 });
    tsp.push_back({ 40, 40 });
    tsp.push_back({ 50, 40 });
    tsp.push_back({ 50, 50 });
    tsp.push_back({ 60, 50 });
    tsp.push_back({ 60, 60 });
    if (cancelled)
        return tsp;

    return tsp;
}
