#include "utils.h"

#include <random>
#include <algorithm>
#include <queue>

using std::max;
using std::min;
using std::pair;

// return a random number between l and h
// mersenne twister in action!!
int genRand(int l, int h) {

	// obtain a random number from hardware
	std::random_device rd;
	// seed
	std::mt19937 eng(rd());
	// set the range
	std::uniform_int_distribution<> distr(l, h);

	return distr(eng);
}

// for doubles
double genRand(double l, double h) {

	// obtain a random number from hardware
	std::random_device rd;
	// seed
	std::mt19937 eng(rd());
	// set the range
	std::uniform_real_distribution<> distr(l, h);

	return distr(eng);
}

// Given three collinear points p, q, r, the function checks if
// point q lies on line segment 'pr'
static bool onSegment(vec2 p, vec2 q, vec2 r)
{
	if (q.x <= max(p.x, r.x) && q.x >= min(p.x, r.x) &&
		  q.y <= max(p.y, r.y) && q.y >= min(p.y, r.y))
	    return true;

	return false;
}

// To find orientation of ordered triplet (p, q, r).
// The function returns following values
// 0 --> p, q and r are colinear
// 1 --> Clockwise
// 2 --> Counterclockwise
static int orientation(vec2 p, vec2 q, vec2 r)
{
	// See https://www.geeksforgeeks.org/orientation-3-ordered-points/
	// for details of below formula.
	double val = (q.y - p.y) * (r.x - q.x) -
			         (q.x - p.x) * (r.y - q.y);

	if (val == 0) return 0; // collinear

	return (val > 0) ? 1 : 2; // clock or counterclock wise
}

// The main function that returns true if line segment 'p1q1'
// and 'p2q2' intersect.
bool doIntersect(vec2 p1, vec2 q1, vec2 p2, vec2 q2)
{
  // spare the stitches that are contiguous
  if (p1 == p2 || p1 == q2) return false;

  if (q1 == p2 || q1 == q2) return false;

	if (p1.x == q1.x && q1.x == p2.x && p2.x == q2.x) return false;

	if (p1.y == q1.y && q1.y == p2.y && p2.y == q2.y) return false;

	// Find the four orientations needed for general and
	// special cases
	int o1 = orientation(p1, q1, p2);
	int o2 = orientation(p1, q1, q2);
	int o3 = orientation(p2, q2, p1);
	int o4 = orientation(p2, q2, q1);

	// General case
	if (o1 != o2 && o3 != o4)
		return true;

	// Special Cases
	// p1, q1 and p2 are colinear and p2 lies on segment p1q1
	if (o1 == 0 && onSegment(p1, p2, q1)) return true;

	// p1, q1 and q2 are colinear and q2 lies on segment p1q1
	if (o2 == 0 && onSegment(p1, q2, q1)) return true;

	// p2, q2 and p1 are colinear and p1 lies on segment p2q2
	if (o3 == 0 && onSegment(p2, p1, q2)) return true;

	// p2, q2 and q1 are colinear and q1 lies on segment p2q2
	if (o4 == 0 && onSegment(p2, q1, q2)) return true;

	return false; // Doesn't fall in any of the above cases
}

void findClosestPair(vec2 m, vec2 &u, vec2 &v, const std::vector<vec2> &points) {

	u = points[0];
	v = points[1];

	// min and second min length
	float min = glm::length(u - m);
	float smin = glm::length(v - m);

	if (min > smin) { std::swap(min, smin); std::swap(u, v); }

	// u - min, v - smin

	for (int i = 2; i < points.size(); ++i) {

		vec2 point = points[i];

		float len = glm::length(point - m);

		if (len < min) {
			u = point; min = len;
		}
		else if (len < smin) {
			v = point; smin = len;
		}

	}
}

float cost1(vec2& u, vec2& v, vec2& n) {

    float distance = glm::length(v - u);
    float cosangle = 0.0;

    if (n.x != 0.0 || n.y != 0.0) {
        vec2 e1 = glm::normalize(v - u);
        vec2 e2 = glm::normalize(n);
        cosangle = fabs(glm::dot(e1, e2));
    }

    return -pow(alpha1, -distance) * pow(beta1, cosangle);
}

// cost function for our TSP problem
float cost(vec2& u, vec2& v, vec2& w) {

    // set both the control parameters as equal
    // both length of the stitch and angle between stitches
    // have an equal weight
    float result = 0.0f;

    vec2 e1 = glm::normalize(u - v);
    vec2 e2 = glm::normalize(w - v);
    float cosangle = abs(glm::dot(e1, e2));
    float distance = glm::length(w - v);

    result = -pow(alpha2, -distance) * beta1 * pow(beta2, -cosangle);
    return result;
}

class comp {

public:
    bool operator()(pair<float, float> a,
                    pair<float, float> b)
    {
        float x1 = a.first * a.first;
        float y1 = a.second * a.second;
        float x2 = b.first * b.first;
        float y2 = b.second * b.second;

        // return true if distance
        // of point 1 from origin
        // is greater than distance of
        // point 2 from origin
        return (x1 + y1) > (x2 + y2);
    }
};

// Function to find the K closest points
void kClosestPoints(float x[], float y[],
                    int n, int k,
										vec2 v,
										std::vector<vec2> &closest)  {

    // Create a priority queue
    std::priority_queue<pair<float, float>,
                   			std::vector<pair<float, float> >,
                   			comp> pq;

    // Pushing all the points
    // in the queue
    for (int i = 0; i < n; i++) {
        pq.push(std::make_pair(x[i] - v.x, y[i] - v.y));
    }

    // Print the first K elements
    // of the queue
    for (int i = 0; i < k; i++) {

        // Store the top of the queue
        // in a temporary pair
        pair<float, float> p = pq.top();

        // Print the first (x)
        // and second (y) of pair
				closest.push_back(vec2(p.first + v.x, p.second + v.y));
        // Remove top element
        // of priority queue
        pq.pop();
    }
}

// get d closest points to point v
void getClosestD(vec2 v,
								 const std::vector<vec2> &points,
                 std::vector<vec2> &closest,
                 size_t d) {

	if (d == 1) return;

	size_t n = points.size();

	float* xValues = new float[n];
	float* yValues = new float[n];

	for (int i = 0; i < n; ++i) {
		xValues[i] = points[i].x;
		yValues[i] = points[i].y;
	}

	kClosestPoints(xValues, yValues, n, d, v, closest);

	delete[] xValues;
	delete[] yValues;
}
