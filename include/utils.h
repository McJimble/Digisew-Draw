#ifndef UTILS_H
#define UTILS_H

#include "glm/vec2.hpp"
#include "glm/gtx/transform.hpp"
#include <utility>
#include <functional>
#include <vector>
#include <algorithm>

#define PI 3.14159

using glm::vec2;

int genRand(int l, int h);

double genRand(double l, double h);

class HashVec {
public:

  float operator() (const vec2& v) const {
    // using the cantor function!
    // return 0.5 * (v.x + v.y) * (v.x + v.y + 1) + v.y;
    return 100 * v.x + v.y;
  }
};

class VecPair {

public:

  vec2 vertex;
  float c; // cost from source

  VecPair(vec2 v, float c): vertex(v), c(c) {}
};

class VecComparator {

public:
  int operator() (const VecPair& v1, const VecPair& v2) {
    return v1.c > v2.c;
  }
};

// A hash function used to hash a pair of any kind
struct hash_pair {
    template <class T1, class T2>
    size_t operator()(const std::pair<T1, T2>& p) const
    {
        size_t pf = 10000 * p.first;
        size_t ps = 1000 * p.second;
        return pf + ps;
    }
};

// does segment (p1, q1) intersect with segment (p2, q2)?
bool doIntersect(vec2 p1, vec2 q1, vec2 p2, vec2 q2);

// set (u, v)
void findClosestPair(vec2 m, vec2 &u, vec2 &v, const std::vector<vec2> &points);

void getClosestD(vec2 v,
                 const std::vector<vec2> &points,
                 std::vector<vec2> &closest,
                 size_t d);

#endif
