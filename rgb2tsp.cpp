//
// Background thread processing of RGB image into <vector> of points in TSP order
//
#include "rgb2tsp.hpp"
#include <chrono>

using namespace std::chrono;

Path mat_to_tsp(cv::Mat& image, const std::atomic_bool& cancelled)
{
    Path tsp;

    // lighten image to blow out the face highlights
    image.convertTo(image, -1, 200);
    if (cancelled)
        return tsp;

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

/*
    image = ImageAdjust[image, {0,0.9}] (* lighten the image to blow out the face highlights *)
    if (cancelled)
        return tsp;
    image = ColorConvert[image,"Grayscale"]
    if (cancelled)
        return tsp;
    image = ColorQuantize[image, 2]
    if (cancelled)
        return tsp;
    image = ImageAdjust[image] (* push colors to 0 or 255 *)
    if (cancelled)
        return tsp;
    pos = PixelValuePositions[image,0]; (* collect positions of all black pixels *)
    if (cancelled)
        return tsp;
    res = FindShortestTour[pos]; (* Use TSP to find path between all black pixels *)
*/

    return tsp;
}
