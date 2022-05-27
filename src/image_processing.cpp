/*
  main driver program for the assignment
  contains all the user interaction stuff using GLUT and
  procedures for reading and writing an image to and from the display
  to disk and vice versa
*/

#include <OpenImageIO/imageio.h>
#include <iostream>
#include "Image.h"
#include "VoronoiDiagramGenerator.h"

#include "utils.h"

#include <vector>
#include <deque>
#include <list>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <sstream>

#ifdef __APPLE__
#  pragma clang diagnostic ignored "-Wdeprecated-declarations"
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#include "edge.h"
#include "glm/ext.hpp"
#include "glm/vec2.hpp" // glm::vec2
#include "glm/gtx/transform.hpp"

using glm::vec2;
using std::unordered_map;

using namespace std;
OIIO_NAMESPACE_USING

// window dimensions
#define WIDTH 512
#define HEIGHT 512

static int ipicture = 0;

// keep track of the current window size at all times
int windowWidth = WIDTH;
int windowHeight = HEIGHT;

int num;  // number of command line args specified
char** imagenames;  // list of image names supplied in the command line args

string currentImageName = "";  // the name of the image on file currently being displayed

Image *picture = NULL;  // a reference stored to the Image object

vector<edge> nogos;  // the no-go line segments
int nCount = 0;  // no go point count

// a bunch of other globals
bool doubleStitch = true;
string normalMap;
int subgridSize;
string dstName;

/*
  read an image from the file whose name is specified in the argument.
  if no name is provided, ask the user for a file name.
  store all the read image information into the image object 'picture'
*/
void readimage(string name="") {

    string inputfilename;

    if (name.empty()) {
        // ask the user the name of file(current working director default) to be read
        cout << "enter input image filename: ";
        cin >> inputfilename;
    }
    else
        inputfilename = name;

    // string filepath = "/home/abhinit/Documents/codeblocks/test/images/" + inputfilename;
    currentImageName = inputfilename;  // set the current image name

    // read the image
    unique_ptr<ImageInput> input = ImageInput::open(inputfilename);
    if (! input) {
        cerr << "Could not read image " << inputfilename << ", error = " << geterror() << endl;
        return;
    }

    const ImageSpec &spec = input->spec();
    // get the metadata for the image(dimensions and number of channels)
    int width = spec.width;
    int height = spec.height;
    int channels = spec.nchannels;

    // allocate space in memory to store the image data
    //unsigned char pixmap[channels * width * height];  // <- windows compilers don't like this.. replaced with below
    unsigned char* pixmap = new unsigned char[channels * width * height];

    if (!input->read_image(TypeDesc::UINT8, pixmap)) {
        cerr << "Could not read image " << inputfilename << ", error = " << geterror() << endl;
        return;
    }
    // close the file handle
    if (!input->close()) {
      cerr << "Could not close " << inputfilename << ", error = " << geterror() << endl;
      return;
    }

    // copy the pixmap into the image
    picture = new Image(width, height, channels);
    picture->copyImage(pixmap);   // make a deep copy of the pixmap
    delete[] pixmap;
}

/*
    Routine to write the current framebuffer to an image file
*/
void writeimage(){

    if (!picture)
        return;

    int w = picture->getWidth();
    int h = picture->getHeight();

    string outfilename;

    // get a filename for the image. The file suffix should indicate the image file
    // type. For example: output.png to create a png image file named output
    cout << "enter output image filename: ";
    cin >> outfilename;

    // create the oiio file handler for the image
    unique_ptr<ImageOutput> outfile = ImageOutput::create(outfilename);
    if(!outfile){
        cerr << "Could not create output image for " << outfilename << ", error = " << geterror() << endl;
        return;
    }

    // open a file for writing the image. The file header will indicate an image of
    // width w, height h, and 4 channels per pixel (RGBA). All channels will be of
    // type unsigned char
    ImageSpec spec(w, h, 4, TypeDesc::UINT8);
    if(!outfile->open(outfilename, spec)){
        cerr << "Could not open " << outfilename << ", error = " << geterror() << endl;
        return;
    }

    // write the image to the file. All channel values in the pixmap are taken to be
    // unsigned chars
    if(!outfile->write_image(TypeDesc::UINT8, picture->getPixmap())){
        cerr << "Could not write image to " << outfilename << ", error = " << geterror() << endl;
        return;
    }
    else
        cout << "Saved successfully." << endl;

    // free up space associated with the oiio file handler
    if (!outfile->close()) {
      cerr << "Could not close " << outfilename << ", error = " << geterror() << endl;
      return;
    }
}
/*
   Reshape Callback Routine: sets up the viewport and drawing coordinates
   This routine is called when the window is created and every time the window
   is resized, by the program or by the user
*/
void handleReshape(int w, int h) {

    // update the width and height info for the display window
    windowWidth = w;
    windowHeight = h;

    // set the viewport to be the entire window
    glViewport(0, 0, w, h);

    // define the drawing coordinate system on the viewport
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
}

/*
  this is the main display routine
  using pixelZoom to always fit the image into the display window
  in the end, we flip the image so that it doesn't display upside down
*/
void drawImage() {

    if (picture) {

        glClear(GL_COLOR_BUFFER_BIT);  // clear window to background color

        int width = picture->getWidth();
        int height = picture->getHeight();

        // ensure that the image is centered
        glRasterPos2i(0, 0);

        // zoom the image according to the window size
        double xr = windowWidth / (double)width;
        double yr = windowHeight / (double)height;
        glPixelZoom(xr, yr);

        // flip the image so that we can see it straight
        Image* flipped = picture->flip();
        glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, flipped->getPixmap());
        flipped->destroy();
        delete flipped;

        glFlush();
    }
}

bool isContiguous(const vector<edge> &graph) {

  // (u1, v1), (u2, v2) -> u2 = v1
  vec2 u1 = graph[0].u; vec2 v1 = graph[0].v;
  for (int i = 1; i < graph.size(); ++i) {
    if (graph[i].u != graph[i - 1].v) {
        printf("%f, %f\n", graph[i].u.x, graph[i].u.y);
        printf("%f, %f\n", graph[i - 1].u.x, graph[i - 1].u.y);
        return false;
    }
  }
  // all match
  return true;
}

// overloaded
bool isContiguous(const deque<edge> &graph) {

  // (u1, v1), (u2, v2) -> u2 = v1
  vec2 u1 = graph[0].u; vec2 v1 = graph[0].v;
  for (int i = 1; i < graph.size(); ++i) {
    if (graph[i].u != graph[i - 1].v)
        return false;
  }
  // all match
  return true;
}

// cost of edge (u, v)
float cost2(vec2 &u, vec2 &v, vec2 &n) {

  float alpha, beta;
  alpha = 1.5f;
  beta = 2.0f;

  vec2 e1 = glm::normalize(v - u);
  vec2 e2 = glm::normalize(n);
  float cosangle = fabs(glm::dot(e1, e2));
  float distance = glm::length(v - u);

  // return -pow(alpha, -distance);
  // return -1;
  // return -pow(alpha, -distance) * pow(beta, cosangle);
  // return -pow(beta, cosangle);
  return -pow(alpha, -distance) * pow(beta, cosangle) + beta;
}

// v - u -> potential stitch direction
// n -> preferred direction
float cost1(vec2 &u, vec2 &v, vec2 &n) {

  float distance = glm::length(v - u);
  float cosangle = 0.0;

  if (n.x != 0.0 || n.y != 0.0) {
    vec2 e1 = glm::normalize(v - u);
    vec2 e2 = glm::normalize(n);
    cosangle = fabs(glm::dot(e1, e2));
  }

  return -pow(alpha1, -distance) * pow(beta1, cosangle);
  // return -pow(beta, cosangle);
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
    // float distance = abs(w.x - v.x) + abs(w.y - v.y);

    result = -pow(alpha2, -distance) * beta1 * pow(beta2, -cosangle);
    return result;
}

// choose the start with the max min distance to any other node of a different intensity
vec2 chooseStart(const vector<vec2> &points, const vector<unsigned char> &density) {

  // # of points
  int num = points.size();

  // select all points with the smallest density
  vector<int> index(num, -1);

  int k = -1; // current index
  unsigned char bestMax = 0;

  for (int i = 0; i < num; ++i) {
    if (density[i] > bestMax) {
      // zero out everything
      bestMax = density[i];
      k = 0;
      index[k] = i;
    }
    else if (density[i] == bestMax)  // update
      index[++k] = i;
  }

  // which of these k is the best one to pick
  int bestIndex = -1;
  float maxClosestDistance = std::numeric_limits<float>::min();

  // iterate for the k smallest elements
  for (int i = 0; i < k + 1; ++i) {
    int m = index[i];
    // find out min distance from current point to all other points of a
    // different intensity
    vec2 p = points[m];

    float closest = std::numeric_limits<float>::max();

    for (int j = 0; j < points.size(); ++j) {
      if (density[j] == density[m]) continue;

      // compare distance
      vec2 k = points[j];

      if (glm::length(p - k) < closest)
        closest = glm::length(p - k);
    }

    // update best distance
    if (closest > maxClosestDistance) {
      bestIndex = m;
      maxClosestDistance = closest;
    }
  }

  return points[bestIndex];
}

/*
   This routine is called every time a key is pressed on the keyboard
*/
void handleKey(unsigned char key, int x, int y) {

    switch(key) {
        case 'r':
        case 'R':
            readimage();
            if (picture)
                glutPostRedisplay();
            break;
        case 'w':
        case 'W':
            writeimage();
            break;
        case 'p':
        case 'P':

            if (picture) {
              Image *density = picture;

              int width = density->getWidth();
              int height = density->getHeight();

              // reset for now
              density->init(0);

              int totalNodes = density->countBlacks();

              printf("total: %d\n", totalNodes);

              // read in the normals
              readimage("normal_map1_small.png");

              const vector<vec2> normals = picture->interpretNormalMap();

              // the final path
              vector<edge> path;

              // 1. generate graph
              unordered_map<vec2, std::vector<vec2>, HashVec> graph;
              graph = density->genGraph();

              cout << "size = " << graph.size() << "\n";

              // 2. prim
              path = density->prim(graph, normals, cost2);

              // path = density->clipJumps(normals, path, cost2);

              // copy graph into a vector
              vector<edge> graphVector;

              for (edge& e : path)
                graphVector.push_back(e);

              // assert(isContiguous(path) == true);

              // draw the edges to the screen
              float xr = windowWidth / (float)WIDTH;
              float yr = windowHeight / (float)HEIGHT;

              ofstream data("shoe.txt");

              // specify window clear (background) color to be opaque white
              glClearColor(0, 0, 0, 1);
              glClear(GL_COLOR_BUFFER_BIT);  // clear window to background color
              // draw edges
              for (int i = 0; i < graphVector.size(); i += 1) {
                // read consecutive points to draw edge
                float x1 = graphVector[i].u.x;
                float y1 = HEIGHT - graphVector[i].u.y;
                float x2 = graphVector[i].v.x;
                float y2 = HEIGHT - graphVector[i].v.y;

                data << int(x1) << ".000000 " << int(y1) << ".000000\n";

                // scale accord to the window size
                x1 *= xr; x2 *= xr;
                y1 *= yr; y2 *= yr;

                // set the drawing color
                glColor3f(1, 1, 1);
                glBegin(GL_LINES);
                  glVertex2f(x2, y2);
                  glVertex2f(x1, y1);
                glEnd();
              }

              data.close();

              // display all the buffered graphics to the screen
              glFlush();
            }
            break;
        case 'd':
        case 'D':
            // reduce the palette
            if (picture) {
              // std::vector<pixel> palette(2, pixel());
              /*
              palette.push_back(pixel(64, 49, 174, 255));
              palette.push_back(pixel(225, 13, 193, 255));
              palette.push_back(pixel(187, 168, 255, 255));
              palette.push_back(pixel(255, 255, 255, 255));
              palette.push_back(pixel(254, 147, 155, 255));
              palette.push_back(pixel(232, 2, 2, 255));
              palette.push_back(pixel(123, 35, 60, 255));
              palette.push_back(pixel(0, 0, 0, 255));
              palette.push_back(pixel(25, 86, 71, 255));
              palette.push_back(pixel(21, 236, 115, 255));
              palette.push_back(pixel(49, 193, 194, 255));
              palette.push_back(pixel(4, 126, 193, 255));
              palette.push_back(pixel(109, 78, 34, 255));
              palette.push_back(pixel(200, 143, 76, 255));
              palette.push_back(pixel(237, 225, 3, 255));
              */
              // picture->getReducedPalette(palette);

              std::vector<pixel> palette;
              palette.push_back(pixel(255, 255, 255, 255));
              palette.push_back(pixel(0, 0, 0, 255));

              picture->reducePalette(palette);
              glutPostRedisplay();
            }
            break;

        case 'c':
        case 'C':
            if (picture) {
              std::vector<pixel> colors(16, pixel());  // 16 colors
              picture->getReducedPalette(colors);

              for (int i = 0; i < 16; ++i)
                std::cout << "(" << (int)colors[i].r << ", " << (int)colors[i].g << ", " << (int)colors[i].b << ")\n";
            }
            break;

        case 'z':
        case 'Z':
          if (picture) {
            picture->sample(5000);
            glutPostRedisplay();
          }
          break;
        case 's':
        case 'S':
          if (picture) {
            // store the density map somewhere
            Image *density = picture;

            int width = density->getWidth();
            int height = density->getHeight();

            // get the total number of vertices
            // int totalNodes = width * height;

            // sample a little differently
            // density->sample(8);  // sample 3 X 3

            // reset for now
            // density->init(0);

            // density->setpixel(99, 0, pixel(0, 0, 0, 255));
            // density->setpixel(0, 0, pixel(0, 0, 0, 255));
            // density->setpixel(50, 50, pixel(0, 0, 0, 255));
            // density->setpixel(0, 99, pixel(0, 0, 0, 255));

            /*
            for (int i = 0; i < 50; ++i) {
              int x = genRand(0, 99);
              int y = genRand(0, 99);
              density->setpixel(y, x, pixel(0, 0, 0, 255));
            }
            */

            int totalNodes = density->countBlacks();

            printf("total: %d\n", totalNodes);

            // read in the normals
            readimage("shoe_rand.png");

            const vector<vec2> normals = picture->interpretNormalMap();

            // threshold for 2-opt
            const float threshold = 25.0f;

            // the final path
            deque<edge> graph;
            // start node
            vec2 start = density->chooseRandom();
            // vec2 start(30, 0);

            Image *densityCopy = new Image(width, height, 4);
            densityCopy->copyImage(density->getPixmap());

            do {

              /*
              vec2 end;

              do {
                end = density->chooseRandom();
              }
              while (start.x == end.x && start.y == end.y);

              // cover all nodes
              // get the intermediate path
              deque<edge> inter = density->dijkstra(normals, cost2,
                                                    start.x, start.y, end.x, end.y, true);
              */
              deque<edge> inter = density->dijkstra(normals, cost2,
                                                    start.x, start.y, -1, -1);
              // add this path to the main graph
              graph.insert(graph.end(), inter.begin(), inter.end());
              // update to new start
              start = graph[graph.size() - 1].v;
            }
            while (graph.size() != totalNodes - 1); // a path will have 'n' vertices and 'n-1' edges

            /*
            graph = density->dijkstra(normals, cost2,
                                      start.x, start.y, -1, -1);
            */

            // copy graph into a vector
            vector<edge> graphVector;

            for (edge& e : graph)
              graphVector.push_back(e);

            /*
            int size = graph.size();

            // freedom for perturbation term
            const int freedom = 3;

            for (edge& e : graph) {

              vec2& u = e.u;
              vec2& v = e.v;

              u.x = freedom * u.x + 1; u.y = freedom * u.y + 1;
              v.x = freedom * v.x + 1; v.y = freedom * v.y + 1;
            }

            for (int i = 0; i < size; ++i) {

              // current edge
              edge e = graph[i];

              vec2& u = e.u;
              vec2& v = e.v;

              int r = genRand(1, 1);

              // perturb randomly in neighborhood
              if (r == 1) {
                // perturb v
                int px = genRand(-freedom/2, freedom/2);
                int py = genRand(-freedom/2, freedom/2);

                // painful edge check
                if (v.x + px > -1 && v.x + px < freedom * width) v.x += px;
                if (v.y + px > -1 && v.y + py < freedom * height) v.y += py;

                // maintain continuity, adjust the next stitch/edge as well
                if (i < size - 1)
                  graph[i + 1].u = v;
              }

              // reduce back
              u.x = (u.x - 1)/freedom; u.y = (u.y - 1)/freedom;
              v.x = (v.x - 1)/freedom; v.y = (v.y - 1)/freedom;

              graphVector.push_back(e);
            }
            */

            // run 2-opt to cut out the long edges
            Image::Opt2H(graphVector, threshold);

            Image::Opt2H(graphVector);

            // reset density
            // density->init(0);

            // graphVector = densityCopy->clipJumps(normals, graphVector, cost2);

            // test path
            assert(isContiguous(graphVector) == true);

            // draw the edges to the screen
            float xr = windowWidth / (float)WIDTH;
            float yr = windowHeight / (float)HEIGHT;

            ofstream data("shoe.txt");

            // specify window clear (background) color to be opaque white
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);  // clear window to background color
            // draw edges
            for (int i = 0; i < graphVector.size(); i += 1) {
              // read consecutive points to draw edge
              float x1 = graphVector[i].u.x;
              float y1 = HEIGHT - graphVector[i].u.y;
              float x2 = graphVector[i].v.x;
              float y2 = HEIGHT - graphVector[i].v.y;

              data << int(x1) << ".000000 " << int(y1) << ".000000\n";

              // scale accord to the window size
              x1 *= xr; x2 *= xr;
              y1 *= yr; y2 *= yr;

              // set the drawing color
              glColor3f(1, 1, 1);
              glBegin(GL_LINES);
                glVertex2f(x2, y2);
                glVertex2f(x1, y1);
              glEnd();
            }

            data.close();

            // display all the buffered graphics to the screen
            glFlush();
            // picture = density;
            // glutPostRedisplay();
          }
          break;
        case 'g':
        case 'G':
            if (picture) {
              /*
              const float threshold = 15.0f;
              vector<edge> graph = picture->computeGraph(cost, threshold);

              cout << "contiguous: " << isContiguous(graph) << "\n";
              */

              // vector<edge> graph = picture->interpretNormalMap();

              // num of partitions
            	int k = 100;
            	// percent of values to be selected from sub-matrix
            	int percent = 100;
              // set a threshold for 2-opt later
              const float threshold = 10.0f;
              // store all the graph nodes
              vector<edge> graph;

              Image *dither = picture;
              // dither->init(0);

              // also keep track of the normals
              readimage(normalMap);

              // picture->randomizeMap();

              vector<vec2> normals = picture->interpretNormalMap();

              /*
            	// get the total count of 1s in the big matrix
            	int totalCount = dither->countBlacks();

              cout << "count = " << totalCount << "\n";
              while (totalCount) {
                // for every sub-matrix
            		for (int i = 0; i < dither->getHeight(); i += k) {
            			for (int j = 0; j < dither->getWidth(); j += k) {
            				dither->genRandNodes(randPic, i, j, k,
                                         percent, totalCount);
            			}
            		}

            		// cout << "count = " << totalCount << "\n";
                randPic->plan(graph, normals, cost);
            		// flush the randPic
                randPic->init();
              }
              */

              // std::vector<vec2> nodes;

              std::vector<vec2> points;
              ifstream infile("points_high.txt");

              float x, y;
              while (infile >> x >> y) {
                if (x < 0.0 || x > 99.0) continue;
                if (y < 0.0 || y > 99.0) continue;
                points.push_back(vec2(x, y));
              }

              infile.close();

              /*
              // store the corresponding intensity values of the points
              std::vector<unsigned char> densityPoints;

              std::vector<vec2> nodes = dither->genPoints(densityPoints, subgridSize);

              // save nodes to disk
              ofstream data("points.txt");

              for (int i = 0; i < nodes.size(); ++i) {
                vec2 point = nodes[i];
                data << point.x << " " << point.y << "\n";
              }

              data.close();
              */

              dither->plan(graph, points, normals, cost1);

              // graph = dither->computeGraph(cost1, 100.0);

              // graph = dither->clipJumps(normals, graph, cost2);

              cout << "num: " << graph.size() << "\n";

              // run 2-opt to cut out the long edges
              Image::Opt2H(graph, threshold);

              std::unordered_map<vec2, std::list<vec2>, HashVec> adj;
              Image::convertToAdj(adj, graph);

              picture->reverseNormalMap(adj, normals);

              vector<pixel> v1 = picture->readImageIntoBuffer();

              Image *reversedMap = picture;

              readimage(normalMap);

              vector<pixel> v2 = picture->readImageIntoBuffer();

              pixel p = Image::RMSError(v2, v1);

              picture = reversedMap;

              vector<bool> isoff;
              picture->flagOff(isoff, graph, normals);

              // graph = dither->clipJumps(normals, graph, cost1);

              // check if contiguous or not
              // assert(isContiguous(graph) == true);

              /*
              Image *map = new Image(picture->getWidth(),
                                     picture->getHeight(),
                                     4);

              map->init();

              map->reverseNormalMap(graph, picture);

              picture = map;
              */

              // graph = Image::clipJumps(normals, graph, 100, 100);

              // cout << "num: " << graph.size() << "\n";

              /*
              // write out the stitches to an image
              Image *stitches = new Image(2000, 2000, 4);

              stitches->init(0);

              stitches->drawStitches(graph);

              picture = stitches; // imp!
              */

              const int Height = 100;
              const int Width = 100;

              // draw the edges to the screen
              float xr = windowWidth / (float)Width;
              float yr = windowHeight / (float)Height;

              ofstream data(dstName);

              for (int i = 0; i < graph.size(); i += 1) {
                // read consecutive points to draw edge
                float x1 = graph[i].u.x;
                float y1 = (float)Height - graph[i].u.y;
                float x2 = graph[i].v.x;
                float y2 = (float)Height - graph[i].v.y;

                // data << x1 << " " << y1 << "\n";
                std::ostringstream s1;
                s1 << x1;
                std::string sx1(s1.str());

                std::ostringstream s2;
                s2 << y1;
                std::string sy1(s2.str());

                if (i == graph.size() - 1)
                    data << "\"*\", \"END\", " << "\"" << sx1 << "\", " << "\"" << sy1 << "\", " << "0\n";
                else {
                    if (isoff[i])
                      data << "\"*\", \"STITCH\", " << "\"" << sx1 << "\", " << "\"" << sy1 << "\", " << "1\n";
                    else
                      data << "\"*\", \"STITCH\", " << "\"" << sx1 << "\", " << "\"" << sy1 << "\", " << "0\n";
                }
                // scale accord to the window size
                x1 *= xr; x2 *= xr;
                y1 *= yr; y2 *= yr;

                // set the drawing color
                glColor3f(1, 1, 1);
                glBegin(GL_LINES);
                  glVertex2f(x2, y2);
                  glVertex2f(x1, y1);
                glEnd();
              }

              data.close();

              glFlush();
            }
            break;

        case 'h':
        case 'H':
            if (picture) {

              int width = picture->getWidth();
              int height = picture->getHeight();

              // picture->init(200);

              // store the corresponding intensity values of the points
              std::vector<unsigned char> densityPoints;

              std::vector<vec2> points = picture->genPoints(densityPoints,
                                                            subgridSize);

              ofstream pdata("points.txt");

              for (int i = 0; i < points.size(); ++i) {
                vec2 point = points[i];
                pdata << point.x << " " << point.y << "\n";
              }

              pdata.close();

              // create a map for points and their corresponding densities
              std::unordered_map<vec2, unsigned char, HashVec> densities;
        
              // read in the no-gos
              vector<edge> segments;

              cout << "# of points = " << points.size() << "\n";

              // place points in bins/buckets
              std::unordered_map<vec2, vector<vec2>, HashVec> buckets;

              const int SUBREGION_SIZE = 4;

              // init the buckets, 10X10 100 buckets
              for (size_t i = 0; i < width / SUBREGION_SIZE; ++i)
                for (size_t j = 0; j < height / SUBREGION_SIZE; ++j)
                  buckets.insert({vec2(i, j), {}});

              // add points into their respective bins
              for (size_t i = 0; i < points.size(); ++i) {

                vec2 point = points[i];

                // get bin co-ordinates
                float x = std::floor(point.x / (float)SUBREGION_SIZE);
                float y = std::floor(point.y / (float)SUBREGION_SIZE);

                buckets[vec2(x, y)].push_back(point);
              }

              // target SCR
              const float SCR = 0.0;
              float a1, b1, a2, b2;
              float w, scr;

              ifstream inparams("inter_params.txt");

              // setup to compute min
              float diff = std::numeric_limits<float>::max();
              float bw;

              while (inparams >> a1 >> b1 >> a2 >> b2 >> w >> scr) {
                // find right scr
                if (SCR - 0.1 <= scr && scr <= SCR + 0.1) {
                    // check w
                    if (fabs(w - scr) < diff) {
                      // update
                      diff = fabs(w - scr);
                      //alpha1 = a1; beta1 = b1;
                      //alpha2 = a2; beta2 = b2;
                      // update blend
                      bw = w;
                    }
                }
              }

              inparams.close();

              cout << "w = " << bw << "\n";

              readimage(normalMap);

              bw = 0.0;
              beta2 = 5.0;
              picture->blend(bw * 0.5 + 0.5);

              vector<vec2> normals = picture->interpretNormalMap();

              // generate random start

              const vec2 start = points[genRand(0, points.size() - 1)];
              // const vec2 start = chooseStart(points, densityPoints);

              std::unordered_map<vec2, vector<edge>, HashVec> stitchBuckets;

              vector<edge> g = picture->dijkstra(start, buckets, stitchBuckets,
                                                 SUBREGION_SIZE,
                                                 normals, cost1, cost,
                                                 segments);

              // write the dirty stitch plan to a file for a later render
              ofstream ddata("dirty.txt");

              for (int i = 0; i < g.size(); ++i) {
                edge e = g[i];
                ddata << e.u.x << " " << e.u.y << " " << e.v.x << " " << e.v.y << "\n";
              }

              ddata.close();

              picture->reverseNormalMap(g, normals);

              // graph = picture->clipJumps(normals, graph, cost1);

              // fix jumps and get final graph
              std::unordered_map<vec2, std::list<vec2>, HashVec> adj =
              picture->cleanup(normals, g, cost1,
                               buckets, stitchBuckets, SUBREGION_SIZE,
                               segments);

              picture->reverseNormalMap(adj, normals);

              // tree traversal for path generation
              vector<vec2> path;
              picture->genPath(adj, densities, path);

              vector<edge> graph;
              // convert path to edges
              for (int i = 0; i < path.size() - 2; ++i) {
              
                graph.push_back(edge(path[i], path[i + 1]));
              }

              // assert(isContiguous(graph) == true); useless now

              // print stitch count ratio
              float rat = Image::SCR(g);
              cout << "SCR = " << rat << "\n";

              vector<bool> isoff;
              picture->flagOff(isoff, graph, normals);

              glClearColor(0, 0, 0, 1);
              glClear(GL_COLOR_BUFFER_BIT);

              const int Height = 100;
              const int Width = 100;

              float xr = windowWidth / (float)Width;
              float yr = windowHeight / (float)Height;

              vector<pixel> v1 = picture->readImageIntoBuffer();

              Image* reversedMap = picture;

              readimage(normalMap);

              vector<pixel> v2 = picture->readImageIntoBuffer();

              pixel p = Image::RMSError(v2, v1);

              picture = reversedMap;

              ofstream data(dstName);

              for (int i = 0; i < graph.size(); i += 1) {
                // read consecutive points to draw edge
                float x1 = graph[i].u.x;
                float y1 = (float)Height - graph[i].u.y;
                float x2 = graph[i].v.x;
                float y2 = (float)Height - graph[i].v.y;

                // data << x1 << " " << y1 << "\n";
                std::ostringstream s1;
                s1 << x1;
                std::string sx1(s1.str());

                std::ostringstream s2;
                s2 << y1;
                std::string sy1(s2.str());

                if (i == graph.size() - 1)
                    data << "\"*\", \"END\", " << "\"" << sx1 << "\", " << "\"" << sy1 << "\", " << "0\n";
                else {
                    if (isoff[i])
                      data << "\"*\", \"STITCH\", " << "\"" << sx1 << "\", " << "\"" << sy1 << "\", " << "1\n";
                    else
                      data << "\"*\", \"STITCH\", " << "\"" << sx1 << "\", " << "\"" << sy1 << "\", " << "0\n";
                }
                // scale accord to the window size
                x1 *= xr; x2 *= xr;
                y1 *= yr; y2 *= yr;

                // set the drawing color
                glColor3f(1, 1, 1);
                glBegin(GL_LINES);
                  glVertex2f(x2, y2);
                  glVertex2f(x1, y1);
                glEnd();
              }

              data.close();

              // run lib emb convert
              char command[50];

              sprintf (command,
                       "./libembroidery-convert %s %s",
                       dstName.c_str(),
                       (dstName.substr(0, dstName.find(".csv")) + ".dst").c_str());

              // system call
              system(command);

              glFlush();
            }
            break;

        case 'm':
        case 'M':
            {
              // read black pixels from dithered image
              std::vector<vec2> points;

              /*
              for (int h = 0; h < picture->getHeight(); ++h) {
                for (int w = 0; w < picture->getWidth(); ++w) {

                  pixel current = picture->getpixel(h, w);

                  if (current.r == 0) {
                    points.push_back(vec2(w, 100.0 - h) + vec2(genRand(-0.5, 0.5), genRand(-0.5, 0.5)));
                  }
                }
              }
              */

              // range
              double hmax = double(picture->getHeight());
              double wmax = double(picture->getWidth());

              // initial shuffle
              for (int i = 1; i <= 100; ++i) {
                points.push_back(vec2(genRand(0.0, wmax),
                                      genRand(0.0, hmax))
                                );
              }

              // update points in loop
            	size_t count = points.size();

              float* xValues = new float[count];
              float* yValues = new float[count];

              for (int i = 0; i < count; ++i) {
                xValues[i] = points[i].x;
                yValues[i] = points[i].y;
              }

              for (int iter = 0; iter < 100; ++iter) {

                // magic!
                VoronoiDiagramGenerator vdg;
              	vdg.generateVoronoi(xValues, yValues, count, 0, wmax, 0, hmax, 0);

                // compute edge list for every point
              	vdg.resetIterator();

                unordered_map<size_t, vector<vec2>> list;

              	float x1, y1, x2, y2;
                // make graph
                while(vdg.getNext(x1, y1, x2, y2)) {
                  // add edge
                  pair<float, float> pu(x1, y1);
                  pair<float, float> pv(x2, y2);

                  if (pu == pv) continue;

                  size_t u = 1000 * x1 + 100 * y1;
                  size_t v = 1000 * x2 + 100 * y2;

                  list[u].push_back(vec2(x2, y2));
                  list[v].push_back(vec2(x1, y1));
                }

                // store voronoi vertices for all generators
                unordered_map<size_t, vector<vec2>> voronois;
                // (generator, voronoi vertex)

                // store all voronoi vertices in a vector
                vector<vec2> vertices;
                unordered_set<size_t> pv;

                vdg.resetIterator();

                while(vdg.getNext(x1, y1, x2, y2)) {
                  // compute hash
                  size_t u = 1000 * x1 + 100 * y1;
                  size_t v = 1000 * x2 + 100 * y2;

                  if (pv.find(u) == pv.end()) {
                    pv.insert(u);
                    vertices.push_back(vec2(x1, y1));
                  }

                  if (pv.find(v) == pv.end()) {
                    pv.insert(v);
                    vertices.push_back(vec2(x2, y2));
                  }

                }

                // process per vertex (voronoi vertex)
                for (vec2 v : vertices) {

                  // lookup degree for v
                  size_t hv = 1000 * v.x + 100 * v.y;
                  size_t d = list[hv].size();

                  // get closest d number of points
                  vector<vec2> closest;
                  getClosestD(v, points, closest, d);
                  // closest stores generators
                  // write voronoi vertices for all selected generators
                  for (int i = 0; i < closest.size(); ++i) {
                    vec2 c = closest[i];
                    // add voronoi vertex for each generator
                    size_t cv = 1000 * c.x + 100 * c.y;
                    voronois[cv].push_back(v);
                  }
                }

                // finally, use weighted average to update points
                for (int i = 0; i < points.size(); ++i) {
                  // current point
                  vec2 point = points[i];
                  size_t hp = 1000 * point.x + 100 * point.y;

                  vector<vec2> vors = voronois[hp];

                  // new point
                  vec2 up(0.0, 0.0);
                  float totweight = 0.0;

                  for (int s = 0; s < vors.size(); ++s) {
                    // get weight for current point
                    int h = max(0, int(99.0 - vors[s].y));
                    h = min(99, h);
                    int w = max(0, int(vors[s].x));
                    w = min(99, w);

                    float p = float(picture->getpixel(h, w).r);
                    p = 255.0 - p + 100.0;

                    if (vors[s].x < 0.0 || vors[s].x > 99.0) continue;
                    if (vors[s].y < 0.0 || vors[s].y > 99.0) continue;

                    up = up + vors[s];
                    totweight += 1.0;
                  }

                  if (totweight > 0.0) {
                    up = float(1.0 / totweight) * up;
                    points[i] = up;
                  }
                }

                for (int i = 0; i < count; ++i) {
                  xValues[i] = points[i].x;
                  yValues[i] = points[i].y;
                }
              }

            	printf("\n-------------------------------\n");

              const int Height = 100;
              const int Width = 100;

              float xr = windowWidth / (float)Width;
              float yr = windowHeight / (float)Height;

              glClearColor(0, 0, 0, 1);
              glClear(GL_COLOR_BUFFER_BIT);

              VoronoiDiagramGenerator vdg;
              vdg.generateVoronoi(xValues, yValues, count, 0, wmax, 0, hmax, 0);

              // compute edge list for every point
              vdg.resetIterator();

              float x1, y1, x2, y2;
              // make graph
              while(vdg.getNext(x1, y1, x2, y2)) {

                // scale accord to the window size
                x1 *= xr; x2 *= xr;
                y1 *= yr; y2 *= yr;

                // set the drawing color
                glColor3f(1, 1, 1);
                glBegin(GL_LINES);
                  glVertex2f(x2, y2);
                  glVertex2f(x1, y1);
                glEnd();
              }

              glPointSize(2.0);
              for(size_t i = 0; i < count; i++)
              {
                glBegin(GL_POINTS);
                glColor3f( 1.0, 1.0, 0.0 );
                glVertex2f(xValues[i] * xr, yValues[i] * yr);
                glEnd();
              }

              glFlush();
            }
            break;
        case 't':
        case 'T':
            {
              // cost(vec2& u, vec2& v, vec2& w)
              // cost1(vec2 &u, vec2 &v, vec2 &n)
              vec2 v(30, 40);
              vec2 u(30, 43);

              vec2 n(0, 1);
              vec2 p1(30, 20);
              vec2 p2(33, 40);

              // cout << "c1 = " << cost(u, v, p1) << "\n";
              // cout << "c2 = " << cost(u, v, p2) << "\n";

              float step = 2 * PI / 20.0;
              float w = 0.5;

              // cout << "d = " << cost(u, v, p2) << "\n";

              for (int i = 0; i < 5; ++i) {
                // new point
                vec2 f(v.x + 3.0 * cos(step * i), v.y + 3.0 * sin(step * i));
                // squish function value between -1 and 0
                float c1 = cost1(v, f, n);
                float c2 = cost(u, v, f);

                auto s1 = [] (float cn) {
                  float alpha, beta;
                  alpha = 1.1;
                  beta = 4.0;
                  // set
                  // previous range
                  float ps = -beta / alpha;
                  float pe = -1.0 / pow(alpha, 5);

                  assert(cn >= ps && cn <= pe);

                  // new range
                  float ns = -1.0;
                  float ne = 0.0;

                  // formula!
                  return (((cn - ps) * (ne - ns)) / (pe - ps)) + ns;
                };

                auto s2 = [] (float cn) {
                  float alpha, beta;
                  alpha = 1.1;
                  beta = 1.6;
                  // set
                  // previous range
                  float ps = -1.0 / alpha;
                  float pe = -1.0 / (pow(alpha, 5) * beta);

                  assert(cn >= ps && cn <= pe);

                  // new range
                  float ns = -1.0;
                  float ne = 0.0;

                  // formula!
                  return (((cn - ps) * (ne - ns)) / (pe - ps)) + ns;
                };

                float c = (1 - w) * s1(c1) + w * s2(c2);

                cout << "c = " << c << "\n";
                cout << "c1 = " << c1 << ", s1 = " << s1(c1) << "\n";
                cout << "c2 = " << c2 << ", s2 = " << s2(c2) << "\n";
              }
            }
            break;
        case 'v':
        case 'V':
             if (picture)
             {
                /*
                glClearColor(0, 0, 0, 1);
                glClear(GL_COLOR_BUFFER_BIT);

                float xr = windowWidth / (float)WIDTH;
                float yr = windowHeight / (float)HEIGHT;

                vec2 a(80, 40);
                vec2 b(80.2, 10);

                vector<vec2> v = picture->zigZag(a, b, 255, 255, 0.5, -1);
                vector<vec2> v1 = picture->zigZag(b, a, 255, 255, 0.5);

                v.insert(v.end(), v1.begin(), v1.end());

                for (int i = 0; i < v.size() - 1; ++i) {

                  // set the drawing color
                  glColor3f(1, 1, 1);
                  glBegin(GL_LINES);
                    glVertex2f(v[i].x * xr, v[i].y * yr);
                    glVertex2f(v[i + 1].x * xr, v[i + 1].y * yr);
                  glEnd();
                }

                glFlush();
                */

                // read points into vector
                vector<vec2> points;

                ifstream infile("pointsg.txt");

                float x, y;
                while (infile >> x >> y)
                  points.push_back(vec2(x, y));

                infile.close();

                glClearColor(0, 0, 0, 1);
                glClear(GL_COLOR_BUFFER_BIT);

                const int Height = 100;
                const int Width = 100;

                float xr = windowWidth / (float)Width;
                float yr = windowHeight / (float)Height;

                glPointSize(2.0);
                for(size_t i = 0; i < points.size(); i++)
                {
                  glBegin(GL_POINTS);
                  glColor3f( 1.0, 1.0, 0.0 );
                  glVertex2f(points[i].x * xr, (Height - points[i].y) * yr);
                  glEnd();
                }

                /*
                // read points into vector
                vector<edge> stitches;

                ifstream infile("dirty.txt");

                float ux, uy, vx, vy;
                while (infile >> ux >> uy >> vx >> vy)
                  stitches.push_back(edge(vec2(ux, uy), vec2(vx, vy)));

                infile.close();

                glClearColor(0, 0, 0, 1);
                glClear(GL_COLOR_BUFFER_BIT);

                const int Height = 100;
                const int Width = 100;

                float xr = windowWidth / (float)Width;
                float yr = windowHeight / (float)Height;

                for (int i = 0; i < stitches.size(); i += 1) {
                  // read consecutive points to draw edge
                  float x1 = stitches[i].u.x;
                  float y1 = (float)Height - stitches[i].u.y;
                  float x2 = stitches[i].v.x;
                  float y2 = (float)Height - stitches[i].v.y;

                  // scale accord to the window size
                  x1 *= xr; x2 *= xr;
                  y1 *= yr; y2 *= yr;

                  // set the drawing color
                  glColor3f(1, 1, 1);
                  glBegin(GL_LINES);
                    glVertex2f(x2, y2);
                    glVertex2f(x1, y1);
                  glEnd();
                }
                */

                glFlush();
            }
            break;

        case 'k':
        case 'K':
          if (picture)
          {
            ofstream data("nogos.txt");

            for (size_t i = 0; i < nogos.size(); ++i) {
              // read consecutive points to draw edge
              float x1 = nogos[i].u.x;
              float y1 = nogos[i].u.y;
              float x2 = nogos[i].v.x;
              float y2 = nogos[i].v.y;

              data << x1 << " " << y1 << " " << x2 << " " << y2 << "\n";
            }

            data.close();
          }
          break;
        case 'f':
        case 'F':
            if (picture) {
              /*
              std::vector<pixel> palette(1024, pixel());

              picture->getReducedPalette(palette);
              */

              std::vector<pixel> palette;
              palette.push_back(pixel(255, 255, 255, 255));
              palette.push_back(pixel(0, 0, 0, 255));
              picture->floydSteinberg(palette);

              glutPostRedisplay();
            }
            break;

        case 'b':
            if (picture) {

              // store the density map somewhere
              Image *density = picture;

              // read in the normals
              readimage("cone_normal6.jpg");
              const vector<vec2> normals = picture->interpretNormalMap();

              // the final path
              vector<edge> graph = density->planWithDensity(normals, 1, cost2);

              // check if contiguous or not
              assert(isContiguous(graph) == true);

              // draw the edges to the screen
              float xr = windowWidth / (float)WIDTH;
              float yr = windowHeight / (float)HEIGHT;

              // ofstream data("cone_full_dijkstra.txt");

              // specify window clear (background) color to be opaque white
              glClearColor(1, 1, 0, 1);
              glClear(GL_COLOR_BUFFER_BIT);  // clear window to background color
              // draw edges
              for (int i = 0; i < graph.size(); i += 1) {
                // read consecutive points to draw edge
                float x1 = graph[i].u.x;
                float y1 = HEIGHT - graph[i].u.y;
                float x2 = graph[i].v.x;
                float y2 = HEIGHT - graph[i].v.y;

                // data << x1 << ".000000 " << y1 << ".000000\n";

                // scale accord to the window size
                x1 *= xr; x2 *= xr;
                y1 *= yr; y2 *= yr;

                // set the drawing color
                glColor3f(1, 0, 0);
                glBegin(GL_LINES);
                  glVertex2f(x2, y2);
                  glVertex2f(x1, y1);
                glEnd();
              }

              // data.close();

              // display all the buffered graphics to the screen
              glFlush();
            }
            break;
        case '1':
            // red
            if (picture) {
                picture->greyscaleRed();
                cout << "Hit 'o' to get the original image back before doing any other operation\n";
                glutPostRedisplay();
            }
            break;
        case '2':
            // green
            if (picture) {
                picture->greyscaleGreen();
                cout << "Hit 'o' to get the original image back before doing any other operation\n";
                glutPostRedisplay();
            }
            break;
        case '3':
            // blue
            if (picture) {
                picture->greyscaleBlue();
                cout << "Hit 'o' to get the original image back before doing any other operation\n";
                glutPostRedisplay();
            }
            break;
        case 'o':
            // restore the image again
            /*
            if (ipicture >= 0)
                currentImageName = imagenames[ipicture];
            */
            currentImageName = imagenames[1];

            //  read the current image
            readimage(currentImageName);

            if (picture)
                glutPostRedisplay();
            break;
        case 'l':
            {
                float xr = windowWidth / (float)WIDTH;
                float yr = windowHeight / (float)HEIGHT;
                cout << "Enter coords:\n";
                float x1, y1, x2, y2;
                cin >> x1 >> y1;
                cin >> x2 >> y2;
                // scale accord to the window size
                x1 *= xr; x2 *= xr;
                y1 *= yr; y2 *= yr;

                // specify window clear (background) color to be opaque white
                glClearColor(1, 1, 1, 1);
                glClear(GL_COLOR_BUFFER_BIT);  // clear window to background color
                glColor3f(0, 0, 1);
                glBegin(GL_LINES);
                  glVertex2f(x2, y2);
                  glVertex2f(x1, y1);
                glEnd();
                // display all the buffered graphics to the screen
                glFlush();
            }
          break;
        case 'i':
            if (picture) {
                picture->inverse();
                glutPostRedisplay();
            }
            break;
        case 'q':		// q - quit
        case 'Q':
        case 27:		// esc - quit
            exit(0);
        default:		// not a valid key -- just ignore it
            return;
    }
}

// callback to handle mouse click events
void mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
    nCount++;

    // process coordinates (x, y)
    float xf = (float)x;
    float yf = (float)y;

    float xr = (float)WIDTH / (float)picture->getWidth();
    float yr = (float)HEIGHT / (float)picture->getHeight();

    xf /= xr;
    yf /= yr;

    cout << xf << ", " << yf << "\n";

    if (nCount % 2 == 0) {
      nogos[nCount / 2 - 1].v = vec2(xf, yf);
      // draw line
      vec2 u = nogos[nCount / 2 - 1].u;
      picture->drawLineBresenham(u.x, u.y, xf, yf);
      // redraw
      glutPostRedisplay();
    }
    else // create new edge in no-go
      nogos.push_back(edge(vec2(xf, yf), vec2(0, 0)));
	}
}

void handleArrowKeys(int key, int x, int y) {

    // if no command line args are specified, dont do anything
    if (num == 0)
        return;

    switch (key) {
        case 100:
            // left key pressed
            // if on the first picture, go back to the last one
            if (ipicture - 1 < 0)
                ipicture = num - 1;
            else
                ipicture -= 1;
            break;
        case 102:
            // right key pressed
            ipicture = (ipicture + 1) % num;  // cycle through all the images
            break;
    }

    // read the image at the given index
    readimage(imagenames[ipicture]);

    if (picture)
        glutPostRedisplay();
}

/*
   Main program to draw the square, change colors, and wait for quit

   (Commented out because port of this code doesn't need this main anymore.

int main(int argc, char* argv[]) {

    imagenames = argv + 1;
    num = argc - 1;

    normalMap = imagenames[2];
    string sss = imagenames[3];

    // convert subgrid size to int
    string::size_type sz;
    subgridSize = stoi(sss, &sz);

    dstName = imagenames[4];

    // start up the glut utilities
    glutInit(&argc, argv);

    // create the graphics window, giving width, height, and title text
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Stitch Planner");

    readimage(imagenames[1]);  // this will load in the density map image
    // readimage(normalMap);

    // set up the callback routines to be called when glutMainLoop() detects
    // an event
    glutDisplayFunc(drawImage);	  // display callback
    glutKeyboardFunc(handleKey);	  // keyboard callback
    glutReshapeFunc(handleReshape); // window resize callback
    glutSpecialFunc(handleArrowKeys);
    glutMouseFunc(mouse);

    // Routine that loops forever looking for events. It calls the registered
    // callback routine to handle each event that is detected
    glutMainLoop();
    return 0;
}
*/

