//
// Background thread processing of RGB image into <vector> of points in TSP order
//
#include "imgui.h"
#include "rgb2tsp.hpp"
#include <chrono>

using namespace cv;
using namespace std::chrono;

// Stucki halftoning processing
// https://github.com/yunfuliu/pixkit/blob/master/modules/pixkit-image/src/halftoning.cpp
bool Stucki1981(const cv::Mat& src, cv::Mat& dst)
{
	//////////////////////////////////////////////////////////////////////////
	// exception
	if (src.type() != CV_8U) {
		throw std::runtime_error("[Stucki1981] accepts only grayscale image");
		return false;
	}
	if (src.empty()) {
		throw std::runtime_error("[Stucki1981] image is empty");
		return false;
	}

	/////////////////////////////////////////////////////////////////////////
	const float ErrorKernel_Stucki[3][5] = {
		0,	0,	0,	8,	4,
		2,	4,	8,	4,	2,
		1,	2,	4,	2,	1
	};
	const int HalfSize = 2;

	// copy Input to RegImage
	cv::Mat tdst1f = src.clone();
	tdst1f.convertTo(tdst1f, CV_32FC1);

	// processing
	for (int i = 0; i < src.rows; i++) {
		for (int j = 0; j < src.cols; j++) {

			float error;
			if (tdst1f.ptr<float>(i)[j] >= 128) {
				error = tdst1f.ptr<float>(i)[j] - 255;	//error value
				tdst1f.ptr<float>(i)[j] = 255;
			}
			else {
				error = tdst1f.ptr<float>(i)[j];	//error value
				tdst1f.ptr<float>(i)[j] = 0;
			}

			float sum = 0;
			for (int x = 0; x <= HalfSize; x++) {
				if (i + x < 0 || i + x >= src.rows)	continue;
				for (int y = -HalfSize; y <= HalfSize; y++) {
					if (j + y < 0 || j + y >= src.cols)	continue;
					sum += ErrorKernel_Stucki[x][y + HalfSize];
				}
			}

			for (int x = 0; x <= HalfSize; x++) {
				if (i + x < 0 || i + x >= src.rows)	continue;
				for (int y = -HalfSize; y <= HalfSize; y++) {
					if (j + y < 0 || j + y >= src.cols)	continue;
					if (x != 0 || y != 0)
						tdst1f.ptr<float>(i + x)[j + y] += (error * ErrorKernel_Stucki[x][y + HalfSize] / sum);
				}
			}
		}
	}

	tdst1f.convertTo(dst, CV_8UC1);
	return true;
}

Path mat_to_tsp(cv::Mat& image, const std::atomic_bool& cancelled)
{
	Path tsp;

	// image = ImageAdjust[image, {0,0.9}] - lighten the image to blow out the face highlights
	image.convertTo(image, -1, 2.25);
	if (cancelled)
		return tsp;

	// ColorConvert[image,"Grayscale"] - converts the color space of image to the specified color space colspace.
	cvtColor(image, image, COLOR_BGR2GRAY);
	if (cancelled)
		return tsp;

	// Stucki halftoning processing
	Stucki1981(image, image);
	if (cancelled)
		return tsp;

	// image = ImageAdjust[image] - convert image to black and white
	adaptiveThreshold(image, image, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 31, 15);
	if (cancelled)
		return tsp;

	/*
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

#if 0
void colorQuantize(cv::Mat& image, int k)
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
	centers = centers.reshape(image.channels(), centers.rows);
	data = data.reshape(image.channels(), data.rows);

	// replace pixel values with their center value:
	switch (image.channels())
	{
	case 1:
	{
		float* p = data.ptr<float>();
		for (size_t i = 0; i < data.rows; i++) {
			int center_id = labels.at<int>(i);
			p[i] = centers.at<float>(center_id);
		}
		break;
	}

	case 2:
	{
		Vec2f* p = data.ptr<Vec2f>();
		for (size_t i = 0; i < data.rows; i++) {
			int center_id = labels.at<int>(i);
			p[i] = centers.at<Vec2f>(center_id);
		}
		break;
	}

	case 3:
	{
		Vec3f* p = data.ptr<Vec3f>();
		for (size_t i = 0; i < data.rows; i++) {
			int center_id = labels.at<int>(i);
			p[i] = centers.at<Vec3f>(center_id);
		}
		break;
	}
	}

	// back to 2d, and uchar:
	image = data.reshape(image.channels(), image.rows);
	image.convertTo(image, CV_8U);
}
#endif