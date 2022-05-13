#ifndef EDGE_H
#define EDGE_H

#include "glm/vec2.hpp" // glm::vec2
#include "glm/gtx/transform.hpp"

struct edge {

  // directed edge (u, v)
  glm::vec2 u, v;

  float weight() {
    return glm::length(v - u);
  }

  edge() {}

  edge(glm::vec2 u, glm::vec2 v): u(u), v(v) {}
};

#endif
