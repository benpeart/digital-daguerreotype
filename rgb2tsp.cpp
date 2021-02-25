//
// processing of RGB image into <vector> of points in TSP order
//
#include "imgui.h"
#include "rgb2tsp.h"
#include <fstream>
#include <stdlib.h>

using namespace cv;
using namespace std;

// returns a vector of pixel positions in src that exactly match the value val.
std::vector<cv::Point> pixelValuePositions(const cv::Mat& src, uchar val)
{
	std::vector<cv::Point> points;

	if (src.type() != CV_8U) 
	{
		throw std::runtime_error("pixelValuePositions accepts only grayscale image");
	}
	if (src.empty()) 
	{
		throw std::runtime_error("pixelValuePositions image is empty");
	}

	// processing
	for (int i = 0; i < src.rows; i++) 
	{
		for (int j = 0; j < src.cols; j++) 
		{
			uchar tmp = src.ptr<unsigned char>(i)[j];
			if (src.ptr<unsigned char>(i)[j] == val) {
				points.push_back({ j, i });
			}
		}
	}

	return points;
}

// Stucki halftoning processing
// https://github.com/yunfuliu/pixkit/blob/master/modules/pixkit-image/src/halftoning.cpp
bool Stucki1981(const cv::Mat& src, cv::Mat& dst)
{
	//////////////////////////////////////////////////////////////////////////
	// exception
	if (src.type() != CV_8U) {
		throw std::runtime_error("[Stucki1981] accepts only grayscale image");
	}
	if (src.empty()) {
		throw std::runtime_error("[Stucki1981] image is empty");
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
	// BUG BUG: for some reason, this sets a bunch of pixels in the last row to black.
	// Stop one row short to avoid this.
	for (int i = 0; i < src.rows - 1; i++) {
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

// Spawn Concorde to calculate tour between all pixels
std::vector<cv::Point> findShortestTour(Path& points)
{
	int counter = 1;
	ofstream f;
	ifstream g;
	int error;
	Path tour;

	// output file header
	f.open("digital-daguerreotype.tsp", fstream::out);
	f << "NAME: digital daguerreotype" << endl;
	f << "TYPE : TSP" << endl;
	f << "DIMENSION : " << points.size() << endl;
	f << "EDGE_WEIGHT_TYPE : EUC_2D" << endl;
	f << "NODE_COORD_SECTION" << endl;

	// output a valid .tsp file for post processing
	for (Path::iterator i = points.begin(); i != points.end(); ++i)
	{
		f << counter++ << " ";
		f << (*i).x << " ";
		f << (*i).y;
		f << endl;
	}
	f.close();

	// now spawn Concorde to create a tour
	error = system("linkern -Q -t 5 -o digital-daguerreotype.tour digital-daguerreotype.tsp");
	if (error)
		throw std::runtime_error("error spawning linkern");

	// convert the tour file to a Path and return it
	g.open("digital-daguerreotype.tour", fstream::in);
	if (g)
	{
		int count, x, y, z;

		g >> count;
		g >> z;

		for (int i = 0; i < count; i++)
		{
			g >> x;
			g >> y;
			g >> z;

			tour.push_back(points[x]);
			tour.push_back(points[y]);
		}

		g.close();
	}

	return tour;
}


Path mat_to_tsp(cv::Mat& image, const std::atomic_bool& cancelled)
{
	Path points, tsp;

	// image = ImageAdjust[image, {0,0.9}] - lighten the image to blow out the face highlights
	image.convertTo(image, -1, 2.25);
#ifdef _DEBUG
	imshow("convertTo", image);
#endif
	if (cancelled)
		return tsp;

	// ColorConvert[image,"Grayscale"] - converts the color space of image to the specified color space colspace.
	cvtColor(image, image, COLOR_BGR2GRAY);
#ifdef _DEBUG
	imshow("cvtColor", image);
#endif
	if (cancelled)
		return tsp;

	// Stucki halftoning processing
	Stucki1981(image, image);
#ifdef _DEBUG
	imshow("Stucki1981", image);
#endif
	if (cancelled)
		return tsp;

	// collect positions of all black pixels
	points = pixelValuePositions(image, 0);
	if (cancelled)
		return tsp;

	// Use TSP to find shortest continuous path between all black pixels
	return findShortestTour(points);
}
