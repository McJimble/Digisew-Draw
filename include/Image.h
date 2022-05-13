// Header file that defines a class for the Image and
// declarations of all the different operations that you could perform
// on the image

#ifndef IMAGE_H
#define IMAGE_H

#include "pixel.h"
#include "edge.h"
#include "utils.h"

#include <vector>
#include <deque>
#include <list>
#include <unordered_map>
#include <unordered_set>

#include "glm/vec2.hpp" // glm::vec2
#include "glm/gtx/transform.hpp"

using glm::vec2;

class Image {
        // model standard image attributes like specs(dimensions, no of channels),
        // the actual image data and the matrix like interface which holds pointers
        // to each and every scanline of the raster image
private:
        int width, height, channels;
        unsigned char *pixmap;
        unsigned char **matrix;  // access in true matrix style

        // get neighbors for input pixel
        std::vector<vec2> generateNeighbors(vec2& pixel, int ex, int ey);
        // overload
        static std::vector<vec2> generateNeighbors(
          const vec2 &current,
          std::unordered_map<vec2, std::vector<vec2>, HashVec> &buckets,
          const std::unordered_set<vec2, HashVec> &visited,
          const float radius,
          const int BUCKET_SIZE,
          const std::vector<edge> &segments
        );

        void fillNeighbors(int ch, int cw, int length, std::vector<vec2> &neighbors);

        vec2 getSafeNeighbor(
        int ch,
        int cw,
        int length,
        const std::vector<vec2> &normals,
        float(*cost)(vec2 &, vec2 &, vec2 &),
        const std::vector<bool> &visited);

        void genPathUtil(vec2 current,
                         vec2 parent,
                         std::unordered_map<vec2, std::list<vec2>, HashVec> &adj,
                         std::unordered_map<vec2, unsigned char, HashVec> &densities,
                         std::vector<vec2> &path);
public:
        Image(int width, int height, int channels);

        // call to clean up
        void destroy() {
            delete[] matrix;
            delete[] pixmap;
        }
        void copyImage(const unsigned char *pixmap_);
        // define some getters
        int getWidth()       { return width; }
        int getHeight()      { return height; }
        const unsigned char* getPixmap() { return pixmap; }

        // routines to get and set pixel values at the given pixel location(x, y)
        pixel getpixel(int x, int y) {
            unsigned char red = matrix[x][4*y];
            unsigned char green = matrix[x][4*y + 1];
            unsigned char blue = matrix[x][4*y + 2];
            unsigned char alpha = matrix[x][4*y + 3];

            return pixel(red, green, blue, alpha);
        }

        void setpixel(int x, int y, pixel pix) {
            matrix[x][4*y] = pix.r;
            matrix[x][4*y + 1] = pix.g;
            matrix[x][4*y + 2] = pix.b;
            matrix[x][4*y + 3] = pix.a;
        }

        // paint the image white
        void init(unsigned char b=255);

        void inverse();

        // reverse the image for display purposes, returns a new image
        Image* flip();

        // greyscale operations
        void greyscaleRed();
        void greyscaleGreen();
        void greyscaleBlue();

        void toBitmap();
        void reducePalette(std::vector<pixel> &palette);

        // using median cut
        void getReducedPalette(std::vector<pixel> &palette);  // the results will be populated
        // into the palette

        // floyd steinberg dithering
        void floydSteinberg(std::vector<pixel> &palette);

        // count black pixels
        int countNodes();

        void reduceNoise();

        // stitch planning
        // let the user define the cost function
        std::vector<edge> computeGraph(float(*cost)(vec2&, vec2&, vec2&),
                                       const float threshold);

        void drawLineBresenham(int xStart, int yStart, int xEnd, int yEnd);

        // iterative stitch planning
        void plan(std::vector<edge> &graph,
                  std::vector<vec2> &nodes,
                  std::vector<vec2> &normals,
                  float(*cost)(vec2 &, vec2 &, vec2 &));

        // a new approach to density
        std::vector<edge> planWithDensity(const std::vector<vec2> &normals,
                                          int count,
                                          float(*cost)(vec2 &, vec2 &, vec2 &));

        // interpret the input RGB image as a normal map
        // return normals
        std::vector<vec2> interpretNormalMap();

        // retain only blue color channel
        void keepBlue();

        // select random black pixels from the image
        void genRandNodes(Image *randPic, int i, int j, int k,
                          int percent, int &totalCount);

        // count the number of black nodes
        int countBlacks();

        static pixel RMS(std::vector<pixel> &pixels, size_t start, size_t end);

        // RMS error between 2 images
        static pixel RMSError(std::vector<pixel> &p1, std::vector<pixel> &p2);

        std::vector<pixel> readImageIntoBuffer();

        // cut out the long edges
        // should ideally be part of a different class
        static void Opt2H(std::vector<edge> &graph, const float threshold);

        // 2-opt for clipping all stitches that cross over each other
        static void Opt2H(std::vector<edge> &graph);

        // get stitch count ratio
        static float SCR(const std::vector<edge> &graph);

        // reverse stitches to get back the normal map
        void reverseNormalMap(std::vector<edge> &graph,
                              const std::vector<vec2> &normals);

        // flag off-dir stitches
        void flagOff(std::vector<bool> &isoff,
                     const std::vector<edge> &graph,
                     const std::vector<vec2> &normals);

        // convert to an adj representation
        static void convertToAdj(std::unordered_map<vec2, std::list<vec2>, HashVec> &adj,
                                 const std::vector<edge> &graph);

        // overload
        void reverseNormalMap(std::unordered_map<vec2, std::list<vec2>, HashVec> &adj,
                              const std::vector<vec2> &normals);

        // dijkstra!!
        std::deque<edge> dijkstra(const std::vector<vec2> &normals,
                                  float(*cost)(vec2 &, vec2 &, vec2 &),
                                  int sx, int sy,
                                  int ex, int ey,
                                  bool end = false);

        // overload, return adj list representing the Shortest Path Tree
        std::vector<edge> dijkstra(
                                   const vec2 &start,
                                   std::unordered_map<vec2, std::vector<vec2>, HashVec> &buckets,
                                   std::unordered_map<vec2, std::vector<edge>, HashVec> &stitchBuckets,
                                   const int BUCKET_SIZE,
                                   const std::vector<vec2> &normals,
                                   float(*cost1)(vec2 &, vec2 &, vec2 &),
                                   float(*cost2)(vec2 &, vec2 &, vec2 &),
                                   const std::vector<edge>& segments
                                  );

        vec2 getBestNode(const vec2 &v,
        const std::vector<vec2> &normals,
        float(*cost)(vec2 &, vec2 &, vec2 &),
        std::unordered_map<vec2, std::vector<vec2>, HashVec> &buckets,
        std::unordered_map<vec2, std::vector<edge>, HashVec> &stitchBuckets,
        std::unordered_map<vec2, int, HashVec> group,
        const float radius,
        const int BUCKET_SIZE,
        float &bestCost,
        int groupNumber,
        std::vector<edge> &segments
        );

        // cleanup jumps
        std::unordered_map<vec2, std::list<vec2>, HashVec> cleanup(
          const std::vector<vec2> &normals,
          const std::vector<edge> &graph,
          float(*cost)(vec2 &, vec2 &, vec2 &),
          std::unordered_map<vec2, std::vector<vec2>, HashVec> &buckets,
          std::unordered_map<vec2, std::vector<edge>, HashVec> &stitchBuckets,
          const int BUCKET_SIZE,
          std::vector<edge> &segments);

        // clip jump stitches
        std::vector<edge> clipJumps(const std::vector<vec2> &normals,
                                    const std::vector<edge> &graph,
                                    float(*cost)(vec2 &, vec2 &, vec2 &));

        // find all jumps
        std::vector<edge> findJumps(const std::vector<vec2> &normals,
                                    const std::vector<edge> &graph,
                                    float(*cost)(vec2 &, vec2 &, vec2 &));

        // sampling
        void sample(int size);

        // choose random black node as the start
        vec2 chooseRandom();

        void changeNormals(const std::vector<vec2> &points,
                           std::vector<vec2> &normals);

        // convert density map into a graph
        std::unordered_map<vec2, std::vector<vec2>, HashVec> genGraph();

        // overload, buckets is a bin lattice essentially
        std::unordered_map<vec2, std::vector<vec2>, HashVec> genGraph(
          const vec2 &start,
          std::unordered_map<vec2, std::vector<vec2>, HashVec> &buckets,
          const int BUCKET_SIZE,
          const std::vector<vec2> &normals,
          float(*cost)(vec2 &, vec2 &, vec2 &)
        );

        // generate a path from the graph traversal of a Dijsktra SPT
        // store the final path in 'path'
        void genPath(std::unordered_map<vec2, std::list<vec2>, HashVec> &adj,
                     std::unordered_map<vec2, unsigned char, HashVec> &densities,
                     std::vector<vec2> &path);

        // prim!
        std::vector<edge> prim(
          std::unordered_map<vec2, std::vector<vec2>, HashVec> &graph,
          const std::vector<vec2> &normals,
          float(*cost)(vec2 &, vec2 &, vec2 &)
        );

        // add some fuzziness to the normal map
        void randomizeMap();

        void replace();

        // blend normal maps
        void blend(const float c);

        // make better zig-zags
        std::vector<vec2> zigZag(const vec2 &a, const vec2 &b,
                                 const unsigned char da, const unsigned char db,
                                 const float k = 1.0,
                                 const float direction = 1.0);

        // generate points on the density map
        std::vector<vec2> genPoints(std::vector<unsigned char> &densityPoints,
                                    const int SUBGRID_SIZE);

        // draw stitches to a custom image
        void drawStitches(std::vector<edge> &graph);
};

#endif
