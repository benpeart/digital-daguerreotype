//
// Travelling Salesman Problem | Greedy Approach
// https://origin.geeksforgeeks.org/travelling-salesman-problem-greedy-approach/
//

// D:\src\opencv\sources\samples\cpp\travelsalesman.cpp

#include "tsp.hpp"
#include <vector>
#include <map>
#include <climits>

using namespace std;

std::vector<cv::Point> findShortestTour(const std::vector<cv::Point>& points)
{
	std::vector<cv::Point> tsp;

	// 'wait' for findShortestTour a few seconds
//	std::this_thread::sleep_for(std::chrono::seconds(5));
	return points;
}

// Function to find the minimum cost path for all the paths
int findMinRoute(vector<vector<int> > tsp)
{
	int* route;
	int sum = 0;
	int counter = 0;
	unsigned j = 0, i = 0;
	int min = INT_MAX;
	map<int, int> visitedRouteList;

	// Starting from the 0th indexed
	// city i.e., the first city
	visitedRouteList[0] = 1;
	route = new int[tsp.size()];

	// Traverse the adjacency
	// matrix tsp[][]
	while (i < tsp.size() && j < tsp[i].size())
	{

		// Corner of the Matrix
		if (counter >= tsp[i].size() - 1)
		{
			break;
		}

		// If this path is unvisited then
		// and if the cost is less then
		// update the cost
		if (j != i && (visitedRouteList[j] == 0))
		{
			if (tsp[i][j] < min)
			{
				min = tsp[i][j];
				route[counter] = j + 1;
			}
		}
		j++;

		// Check all paths from the
		// ith indexed city
		if (j == tsp[i].size())
		{
			sum += min;
			min = INT_MAX;
			visitedRouteList[route[counter] - 1] = 1;
			j = 0;
			i = route[counter] - 1;
			counter++;
		}
	}

	// Update the ending city in array
	// from city which was last visited
	i = route[counter - 1] - 1;

	for (j = 0; j < tsp.size(); j++)
	{

		if ((i != j) && tsp[i][j] < min)
		{
			min = tsp[i][j];
			route[counter] = j + 1;
		}
	}
	sum += min;
	delete route;

	// Started from the node where we finished as well.
	return sum;
}

#ifdef FALSE

// Driver Code
int main()
{
	// Input Matrix
	vector<vector<int> > tsp = { { -1, 10, 15, 20 },
								{ 10, -1, 35, 25 },
								{ 15, 35, -1, 30 },
								{ 20, 25, 30, -1 } };

	// Function Call
	cout << ("Minimum Cost is : ");
	cout << (findMinRoute(tsp););
}

#endif
