//
// Background thread processing of RGB image into <vector> of points in TSP order
//

#include <opencv2/opencv.hpp>   // Include OpenCV API
#include <vector>
#include <atomic>

typedef std::vector<cv::Point> Path;

extern Path mat_to_tsp(cv::Mat& image, const std::atomic_bool& cancelled);