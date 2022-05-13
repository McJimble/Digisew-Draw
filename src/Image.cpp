#include "Image.h"
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <cinttypes>
#include <limits>
#include <queue>
#include <utility>
#include  <random>
#include  <iterator>
#include <cmath>
#include <cfenv>
#include <climits>

#include "utils.h"

typedef unsigned char uchar;
typedef std::int16_t int16;

typedef std::pair<float, vec2> iPair;

const float INF = std::numeric_limits<float>::max();

Image::Image(int width, int height, int channels) :
width(width), height(height), channels(channels)
{
    int numbytes = 4 * width * height;  // always use 4 channels
    // allocate space for the pixmap
    pixmap = new unsigned char[numbytes];

    // setup the matrix access as well
    matrix = new unsigned char *[height];
    // set all the pointers appropriately
    matrix[0] = pixmap;
    for (int i = 1; i < height; ++i)
        matrix[i] = matrix[i - 1] + 4 * width;
}

// paint white
void Image::init(unsigned char b) {

  for (int h = 0; h < height; ++h)
      for (int w = 0; w < width; ++w)
          setpixel(h, w, pixel(b, b, b, 255));
}

// helper routine to set the start and end points correctly
void order(int start, int end, int &s, int &e) {
	s = std::min(start, end);
	e = (start == s) ? end : start;
}

bool lessTest(int s, int e) {
	return s <= e;
}

bool greaterTest(int s, int e) {
	return s >= e;
}

void Image::drawLineBresenham(
int xStart,
int yStart,
int xEnd,
int yEnd) {

  // vertical line edge case
	if (xStart == xEnd) {
		// slope of infinity
		int start, end;
		order(yStart, yEnd, start, end);

		// start < end
		while (start <= end) {
			setpixel(start, xStart, pixel(0, 0, 0, 255));
			start++;
		}
	}
	else {
		// get the slope as the first step
		float slope = (yStart - yEnd) / (float)(xStart - xEnd);
		int delta = 2 * std::abs(yStart - yEnd);

		// init callback
		bool (*comp)(int, int) = lessTest;

		if (slope >= -1 && slope <= 1) {
				int slope_error = 0;
				// accumulate slope error
				int threshold = std::abs(xStart - xEnd);  // std threshold for bresenham
				// threshold incrementer
				int threshold_incr = 2 * std::abs(xStart - xEnd);

				// move along the direction of x
				int xs, xe;
				order(xStart, xEnd, xs, xe);
				// get the lower y value
				int ys = std::min(yStart, yEnd);

				// move x left or right? depends on slope
				int incr_x = 1;
				if (slope < 0) {
					incr_x = -1;
					// swap if slope is negative
					std::swap(xs, xe);
					comp = greaterTest;
				}

				while (comp(xs, xe)) {
					slope_error += delta;
					// update y if threshold exceeded
					if (slope_error > threshold) {
						ys++;
						threshold += threshold_incr;
					}
					// draw
					setpixel(ys, xs, pixel(0, 0, 0, 255));
					xs += incr_x;
				}
		}
		else {
			int delta = 2 * std::abs(xStart - xEnd);

			// steeper lines
			int slope_error = 0;
			// accumulate slope error
			int threshold = std::abs(yStart - yEnd);  // std threshold for bresenham
			// incrementer for the threshold
			int threshold_incr = 2 * std::abs(yStart - yEnd);

			// move along the direction of y
			int ys, ye;
			order(yStart, yEnd, ys, ye);
			// get the lower x value
			int xs, xe;
			order(xStart, xEnd, xs, xe);

			// move x left or right? depends on slope
			int incr_x = 1;
			if (slope < 0) {
				incr_x = -1;
				// swap the x
				std::swap(xs, xe);
			}

			while (ys <= ye) {
				slope_error += delta;
				// update y if threshold exceeded
				if (slope_error > threshold) {
					xs += incr_x;
					threshold += threshold_incr;
				}
				// draw
				setpixel(ys, xs, pixel(0, 0, 0, 255));
				ys++;
			}

		}

	}
}

// draw a bunch of lines on an image
void Image::drawStitches(std::vector<edge> &graph) {

  for (size_t i = 0; i < graph.size(); ++i) {
    vec2 u = graph[i].u;
    vec2 v = graph[i].v;

    // draw line with pixels
    // be careful with the multiplier here
    drawLineBresenham(5*u.x, 5*u.y, 5*v.x, 5*v.y);
  }
}

// convert the input image to RGBA format if required
void Image::copyImage(const unsigned char *pixmap_) {
    // get the number of bytes to copy
    int numbytes = channels * width * height;

    if (channels == 1) {
        // greyscale image
        for (int i = 0, j = 0; i < numbytes; ++i, j += 4) {
            pixmap[j] = pixmap_[i];
            pixmap[j+1] = pixmap_[i];
            pixmap[j+2] = pixmap_[i];
            pixmap[j+3] = 255;
        }
    }
    else if (channels == 3) {
        // RGB image
        int alphas = 0;  // the number of alphas we have seen till now
        for (int i = 0; i < numbytes; ++i) {
            if ((i + alphas) % 4 == 3) {
                // we've seen an alpha
                pixmap[i + alphas] = 255;
                alphas++;
            }
            pixmap[i + alphas] = pixmap_[i];
        }
    }
    else
        memcpy(pixmap, pixmap_, numbytes);  // vanilla RGBA image, no need to do anything
}

/*
  the following 3 procedures will perform the greyscale operation
  for the 3 colors respectively.

  for eg. greyscaleRed() will copy the red value of each pixel into the other
  2 channels(blue and green) which will end up making all the red pixels white
  and all the non red pixels black(sort of).

  similarly, we can do the same thing for the green and blue color channels as well
*/

void Image::greyscaleRed() {

    // set all the b and g to red
    for (int h = 0; h < height; ++h) {
        for (int w = 0; w < width; ++w) {
            pixel pix = getpixel(h, w);
            pix.b = pix.g = pix.r;
            setpixel(h, w, pix);
        }
    }
}

void Image::greyscaleGreen() {

    // set all the r and b to green
    for (int h = 0; h < height; ++h) {
        for (int w = 0; w < width; ++w) {
            pixel pix = getpixel(h, w);
            pix.r = pix.b = pix.g;
            setpixel(h, w, pix);
        }
    }
}

void Image::greyscaleBlue() {

    // set the r and g to blue
    for (int h = 0; h < height; ++h) {
        for (int w = 0; w < width; ++w) {
            pixel pix = getpixel(h, w);
            pix.r = pix.g = pix.b;
            setpixel(h, w, pix);
        }
    }
}

// flip the image upside down for displaying
Image* Image::flip() {

    // flip the image for displaying
    Image *reversed = new Image(width, height, channels);

    // copy the image row by row, from bottom to top, which ends up
    // flipping it
    for (int h = 0; h < height; ++h) {
        for (int w = 0; w < width; ++w) {
            pixel pix = getpixel(height - h - 1, w);
            reversed->setpixel(h, w, pix);
        }
    }

    return reversed;
}

/*
  the classic invert colors operation
  implemented in exactly the same way as was given in the first quiz
*/
void Image::inverse() {

    // standard inversion operation
    for (int h = 0; h < height; ++h) {
        for (int w = 0; w < width; ++w) {
            pixel pix = getpixel(h, w);
            pix.r = 255 - pix.r;
            pix.g = 255 - pix.g;
            pix.b = 255 - pix.b;
            setpixel(h, w, pix);
        }
    }
}

// dithering baby, will work only for greyscale images though
void Image::toBitmap() {

  for (int h = 0; h < height; ++h) {

    int left_error = 0;  // error is 0 at the start of every scanline

    for (int w = 0; w < width; ++w) {
      // get the current pixel
      pixel pix = getpixel(h, w);

      int intensity = pix.r;
      intensity += left_error;
      left_error = intensity;

      if (255 - intensity < intensity) {
        // closer to white
        left_error = intensity - 255;
        intensity = 255;
      }
      else
        intensity = 0;

      pix.r = pix.g = pix.b = intensity;
      setpixel(h, w, pix);
    }

  }

}

// find the closest color in the palette to the given color
int findClosestPaletteColor(pixel &color,
                            std::vector<pixel> &palette) {

    float smallest = std::numeric_limits<float>::max();
    int palette_index = -1;  // index of the closest color in the palette

    // get the color closest to the current pixel in the palette
    for (int i = 0; i < palette.size(); ++i) {
      pixel palette_color = palette[i];
      pixel difference = color - palette_color;

      // sum the squares of the differences
      float diff =  (float)(difference.r * difference.r) +
                    (float)(difference.g * difference.g) +
                    (float)(difference.b * difference.b);
      // update closest color index
      if (diff < smallest) {
        smallest = diff;
        palette_index = i;
      }
    }

    return palette_index;
}

int16 byteCap(int16 num) {
  if (num > 255) return 255;
  if (num < 0) return 0;

  return num;
}

// floyd-steinberg in action ladies
void Image::floydSteinberg(std::vector<pixel> &palette) {

  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {
      // get the current pixel value from the temp buffer
      pixel oldpixel = getpixel(h, w);

      pixel newpixel = palette[findClosestPaletteColor(oldpixel, palette)];

      setpixel(h, w, newpixel);

      int16 qer = oldpixel.r - newpixel.r;
      int16 qeg = oldpixel.g - newpixel.g;
      int16 qeb = oldpixel.b - newpixel.b;

      // 7, 3, 5, 1 -> 16

      pixel neighbor;
      if (w + 1 < width) {
        neighbor = getpixel(h, w + 1);
        neighbor.r = byteCap(neighbor.r + qer * 7/16);
        neighbor.g = byteCap(neighbor.g + qeg * 7/16);
        neighbor.b = byteCap(neighbor.b + qeb * 7/16);
        setpixel(h, w + 1, neighbor);
      }

      if (h + 1 < height && w - 1 > -1) {
        neighbor = getpixel(h + 1, w - 1);
        neighbor.r = byteCap(neighbor.r + qer * 3/16);
        neighbor.g = byteCap(neighbor.g + qeg * 3/16);
        neighbor.b = byteCap(neighbor.b + qeb * 3/16);
        setpixel(h + 1, w - 1, neighbor);
      }

      if (h + 1 < height) {
        neighbor = getpixel(h + 1, w);
        neighbor.r = byteCap(neighbor.r + qer * 5/16);
        neighbor.g = byteCap(neighbor.g + qeg * 5/16);
        neighbor.b = byteCap(neighbor.b + qeb * 5/16);
        setpixel(h + 1, w, neighbor);
      }

      if (h + 1 < height && w + 1 < width) {
        neighbor = getpixel(h + 1, w + 1);
        neighbor.r = byteCap(neighbor.r + qer * 1/16);
        neighbor.g = byteCap(neighbor.g + qeg * 1/16);
        neighbor.b = byteCap(neighbor.b + qeb * 1/16);
        setpixel(h + 1, w + 1, neighbor);
      }
      // get the neighboring pixels and set their values up accordingly
    }
  }
  std::cout << "Done\n";
}

/*
  reduce palette of the image
*/
void Image::reducePalette(std::vector<pixel> &palette) {

  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {
      // find the closest color and set the pixel accordingly
      pixel current_pixel = getpixel(h, w);
      int palette_index = findClosestPaletteColor(current_pixel, palette);
      setpixel(h, w, palette[palette_index]);
    }
  }

}

// read an image into a vector
std::vector<pixel> Image::readImageIntoBuffer() {

  std::vector<pixel> pixels;

  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {
      // find the closest color and set the pixel accordingly
      pixel current_pixel = getpixel(h, w);
      pixels.push_back(current_pixel);
    }
  }

  return pixels;
}

// I'm gonna make a huge assumption here, p1.size() = p2.size()
pixel Image::RMSError(std::vector<pixel> &p1, std::vector<pixel> &p2) {

  double r, g, b;  // these values will store the square of the sums
	r = g = b = 0;

  // the number of pixels that are actually taken into account
  size_t count = 0;

	// average all the channels up
	for (size_t i = 0; i < p1.size(); ++i) {

		double r1 = (double)p1[i].r;
		double g1 = (double)p1[i].g;
		double b1 = (double)p1[i].b;

    double r2 = (double)p2[i].r;
		double g2 = (double)p2[i].g;
		double b2 = (double)p2[i].b;

    // skip if white pixel
    if (r2 == 255.0 && g2 == 255.0 && b2 == 255.0) continue;

		r += (r1 - r2) * (r1 - r2);
		g += (g1 - g2) * (g1 - g2);
		b += (b1 - b2) * (b1 - b2);

    count++;
	}

	// divide the sum by the number of pixels and take the square root
	r = sqrt(r / (double)count);
	g = sqrt(g / (double)count);
	b = sqrt(b / (double)count);

  printf("r = %f, g = %f, b = %f\n", r, g, b);
  printf("%ld\n", count);

	return pixel((uchar)r, (uchar)g, (uchar)b, 255);
}

// root mean square of all pixel values in the given range
pixel Image::RMS(std::vector<pixel> &pixels, size_t start, size_t end) {

	// get the total number of pixels
	double n = (double)(end - start + 1);

	double r, g, b;  // these values will store the square of the sums
	r = g = b = 0;

	// average all the channels up
	for (size_t i = start; i <= end; ++i) {

		double red = (double)pixels[i].r;
		double green = (double)pixels[i].g;
		double blue = (double)pixels[i].b;

		r += red * red;
		g += green * green;
		b += blue * blue;
	}

	// divide the sum by the number of pixels and take the square root
	r = sqrt(r / n);
	g = sqrt(g / n);
	b = sqrt(b / n);

	return pixel((uchar)r, (uchar)g, (uchar)b, 255);
}

// comparator functions for red, green and blue color channels
bool compareRed(pixel color1, pixel color2) {
	return color1.r < color2.r;
}

bool compareGreen(pixel color1, pixel color2) {
	return color1.g < color2.g;
}

bool compareBlue(pixel color1, pixel color2) {
	return color1.b < color2.b;
}

// return channel with the biggest range
Channel getBiggestRangeChannel(std::vector<pixel> &pixels, size_t start, size_t end) {

	uchar minRed, maxRed;
	uchar minGreen, maxGreen;
	uchar minBlue, maxBlue;

	minRed = minGreen = minBlue = 255;
	maxRed = maxGreen = maxBlue = 0;

	// iterate over pixels[start...end]
	for (size_t i = start; i <= end; ++i) {

		// get the current pixel value
		uchar red = pixels[i].r;
		uchar blue = pixels[i].b;
		uchar green = pixels[i].g;

		// update the mins and maxes appropriately
		if (red < minRed) minRed = red;
		if (green < minGreen) minGreen = green;
		if (blue < minBlue) minBlue = blue;

		if (red > maxRed) maxRed = red;
		if (green > maxGreen) maxGreen = green;
		if (blue > maxBlue) maxBlue = blue;
	}

	// get the maximum ranges for all colors now
	uchar redRange = maxRed - minRed;
	uchar greenRange = maxGreen - minGreen;
	uchar blueRange = maxBlue - minBlue;

	// return the color with the biggest range
	if (redRange > greenRange && redRange > blueRange) return RED;
	if (greenRange > blueRange) return GREEN;
	return BLUE;
}

// utility function to support the median cut procedure
void medianCutUtil(std::vector<pixel> &pixels, std::vector<pixel> &palette,
				   size_t start, size_t end, size_t &paletteIndex, size_t length) {

	// base case:
	if (length == 0) {
		// average all the colors out to get a single color
		pixel& color = palette[paletteIndex++];
		color = Image::RMS(pixels, start, end);
	}
	else {
		// sort based on the color channel with the biggest range
		Channel channel = getBiggestRangeChannel(pixels, start, end);

		bool (*compare)(pixel, pixel);
		// set the appropriate comparator based on the color channel
		if (channel == RED) compare = compareRed;
		else if (channel == GREEN) compare = compareGreen;
		else compare = compareBlue;

		// sort the given range
		std::sort(pixels.begin() + start, pixels.begin() + end + 1, compare);

		// divide into two halves and let the recursion magic begin
		size_t mid = (start + end) / 2;

		// recurse on the left half and then on the right half
		medianCutUtil(pixels, palette, start, mid, paletteIndex, length - 1);
		medianCutUtil(pixels, palette, mid + 1, end, paletteIndex, length - 1);
	}
}

// apply the median cut algorithm, a thin wrapper over the main median cut procedure
void medianCut(std::vector<pixel> &pixels, std::vector<pixel> &palette) {

	size_t length = palette.size();

	size_t paletteIndex = 0;
	medianCutUtil(pixels, palette, 0, pixels.size() - 1, paletteIndex, (size_t)log2((double)length));
}

// reduce the number of colors in the image by applying the median cut algorithm
void Image::getReducedPalette(std::vector<pixel> &palette) {

  std::unordered_set<pixel, HashColor> unique; // store all the unique pixel values

  // iterate over every pixel and add it to the set if it doesn't already exist
  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {
      // get the current pixel
      pixel current = getpixel(h, w);

      // std::cout << "(" << (int)current.r << ", " << (int)current.g << ", " << (int)current.b << ")\n";

  		if (unique.find(current) == unique.end()) {
  			// the element does not exist, add it
  			unique.insert(current);
  		}

    }
  }

  std::vector<pixel> unique_pixels;  // vector containing all the unique pixels

  // print out the contents of the set
  for (const auto& itr : unique) {

    // std::cout << "r = " << (int)itr.r << " g = " << (int)itr.g << " b = " <<
                 // (int)itr.b << " a = " << (int)itr.a << "\n";

    unique_pixels.push_back(pixel(itr.r, itr.g, itr.b, itr.a));
  }

  std::cout << "# of colors in the original image: " << unique_pixels.size() << "\n";

  // let the median cut algorithm begin
  medianCut(unique_pixels, palette);
}

int Image::countNodes() {

  int count = 0;
  // count all black pixels
  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {
      pixel current = getpixel(h, w);
      if (current.r == 0 && current.g == 0 && current.b == 0)
        count++;
    }
  }

  return count;
}

// pushes new node into 'nodes'
// search for the best node from index [start...end]
// where end is just the last index of the nodes array
void pushNewNode(std::vector<vec2> &nodes,
               int start,
               float(*cost)(vec2&, vec2&, vec2&)) {

    // get the first 2 nodes in the stitch
    vec2 u = nodes[start - 2];
    vec2 v = nodes[start - 1];

    float smallest = std::numeric_limits<float>::max();
    int nodeIndex = -1;  // index of the best node
    for (int i = start; i < nodes.size(); ++i) {
        vec2 w = nodes[i];
        // compute cost and compare
        float val = cost(u, v, w);
        if (val <= smallest) {
          // update
          nodeIndex = i;
          smallest = val;
        }
    }
    // swap
    std::swap(nodes[start], nodes[nodeIndex]);
}

// get the edge weight between u and v
float weight(vec2 u, vec2 v,
             int width, int height,
             const std::vector<vec2> &normals,
             float(*cost)(vec2 &, vec2 &, vec2 &)) {

  u.y = std::floor(u.y);
  u.x = std::floor(u.x);

  // get the corresponding normal at 'u'
  vec2 n = normals[u.y * width + u.x];

  // convert to diff system
  u.x -= width/2;
  u.y = height/2 - u.y;

  v.x -= width/2;
  v.y = height/2 - v.y;

  return cost(u, v, n);
}

// find the first edge that exceeds the given threshold
int findLongEdge(std::vector<edge> &graph, const float threshold) {

  int index = -1;
  for (int i = 0; i < graph.size(); ++i) {
    if (graph[i].weight() > threshold) {
      index = i;
      break;
    }
  }
  return index;
}

void Image::Opt2H(std::vector<edge> &graph,
                  const float threshold) {
  // 2-opt heuristic
  int index;
  while ((index = findLongEdge(graph, threshold)) != -1 &&
         index != graph.size() - 1) {
    // long edge
    edge &e = graph[index];

    // for each edge
    for (int i = 0; i < graph.size(); ++i) {
      edge &current = graph[i];

      // skip if same edge!
      if (e.u == current.u && e.v == current.v)
        continue;

      // exchange edges if possible
      if (e.weight() + current.weight() >
          glm::length(e.u - current.u) + glm::length(e.v - current.v)) {

          // better edge found, create a new edge by swapping
          // reverse the intermediate route now (along with the edges)
          int start, end;
          if (index < i) {
            vec2 ev = e.v;
            vec2 cu = current.u;
            e.v = cu;
            current.u = ev;
            start = index + 1;
            end = i - 1;
          }
          else {
            vec2 eu = e.u;
            vec2 cv = current.v;
            current.v = eu;
            e.u = cv;
            start = i + 1;
            end = index - 1;
          }

          while (start < end) {
            // reverse the edges first
            std::swap(graph[start].u, graph[start].v);
            std::swap(graph[end].u, graph[end].v);
            // now swap the edges
            std::swap(graph[start], graph[end]);
            start++; end--;  // move the 2 pointers inward
          }
          if (start == end)
            std::swap(graph[start].u, graph[start].v);
      }
    }
  }

  // clip the last edge if it is still longer
  if (graph[graph.size() - 1].weight() > threshold)
      graph.pop_back();
}

// find a pair of edges that cross over each other
// (i, j) is that pair
bool isCrossing(std::vector<edge> &graph, int &i, int &j) {

  i = j = -1; // init

  for (int k = 0; k < graph.size(); ++k) {
    // current edge
    edge e1 = graph[k];

    for (int l = k + 1; l < graph.size(); ++l) {
      // get segments
      edge e2 = graph[l];

      // check if these edges intersect
      if (doIntersect(e1.u, e1.v, e2.u, e2.v)) {
        // set and return
        i = k;
        j = l;
        return true;
      }
    }
  }

  // no intersections!, we are done
  return false;
}

// adapted for directionality
void Image::Opt2H(std::vector<edge> &graph) {

  int a, b;

  int count = 0;

  while (isCrossing(graph, a, b) && count < 5000) {

    count++;

    // printf("%d\n", count);

    // consistent variable names with what we have above
    edge &current = graph[a];
    edge &e = graph[b];

    // fix crossing by swapping
    // reverse the intermediate route now (along with the edges)
    int start, end;

    // modify, only one condition here, since b > a, always!
    vec2 eu = e.u;
    vec2 cv = current.v;
    current.v = eu;
    e.u = cv;

    start = a + 1;
    end = b - 1;

    while (start < end) {
      // reverse the edges first
      std::swap(graph[start].u, graph[start].v);
      std::swap(graph[end].u, graph[end].v);
      // now swap the edges
      std::swap(graph[start], graph[end]);
      start++; end--;  // move the 2 pointers inward
    }
    if (start == end)
      std::swap(graph[start].u, graph[start].v);
  }
}

std::vector<edge> Image::computeGraph(
  float(*cost)(vec2&, vec2&, vec2&),
  const float threshold
) {
  // store all the computed edges of the graph into a vector
  std::vector<vec2> nodes;

  // load all pixels into the vector that are black in color
  for (int h = 0; h < height; ++h) {
      for (int w = 0; w < width; ++w) {
        pixel current = getpixel(h, w);
        if (current.r == 0 && current.g == 0 && current.b == 0)
            nodes.push_back(vec2(w, h));
      }
  }

  std::swap(nodes[0], nodes[450]);
  std::swap(nodes[1], nodes[520]);

  // re-arrange the nodes to get the desired order of edges
  for (int i = 2; i < nodes.size(); ++i)
    pushNewNode(nodes, i, cost);

  // store all the edges of the graph
  std::vector<edge> graph;
  for (int i = 0; i < nodes.size() - 1; ++i) {
    edge e;
    e.u = nodes[i]; e.v = nodes[i + 1];
    graph.push_back(e);
  }

  Image::Opt2H(graph, threshold);

  return graph;
}

bool isJump(vec2 &u, vec2 &v, const vec2 &n, const int width, const int height) {

  vec2 uu, vu;

  // convert to diff system
  uu.x = u.x - width/2;
  uu.y = height/2 - u.y;

  vu.x = v.x - width/2;
  vu.y = height/2 - v.y;

  vec2 e = glm::normalize(vu - uu);
  vec2 nn = glm::normalize(n);

  return fabs(glm::dot(e, nn)) < 0.9;
  // return fabs(glm::dot(e, nn)) < 0.8;
  // return glm::length(v - u) > 10.0;
}

// make a zig-zagger between a and b, with constant 60 degree angles
// assuming a and b are distinct of course
// k = parameter to control length
// direction = flip control
// da, db -> corresponding densities of a and b
std::vector<vec2> Image::zigZag(const vec2 &a, const vec2 &b,
const unsigned char da, const unsigned char db,
const float k,
const float direction) {

  std::vector<vec2> zigzags;  // the set of zigzags to be returned

  // push the first vertex in
  zigzags.push_back(a);

  // # of zig-zags we wanna make depends on the length of the stitch (a, b)
  float num = k * glm::length(b - a);
  // lerp co-efficient
  float increment = 1.0 / num;

  float alpha = increment;

  vec2 a_ = a; // update at every iteration

  float flip = direction;

  // use the densities of the two endpoints to compute height of the zig-zags
  const float h1 = 0.3;  // min height
  const float h2 = 0.7;  // max height
  float zheight = h1 + ((da + db) / 2.0) * (h2 - h1) / 255.0;

  while (alpha <= 1.0) {

    // get midpoint of (b - a)
    vec2 b_ = (1.0f - alpha) * a + alpha * b;

    vec2 p = 0.5f * a_ + 0.5f * b_; // the actual position vector of p

    // solve for q
    // we know that (q - p) dot (b - a) = 0
    // set y = 5, solve for x or set x = 5 and solve for y
    float x, y;  // these are the coordinates of 'q'

    if (b_.x == a_.x) {
      // vertical line
      x = 5;
      y = (p.x - x) * (b_.x - a_.x) / (b_.y - a_.y) + p.y;
    }
    else {
      // horizontal line or a normal line
      y = 5;
      x = (p.y - y) * (b_.y - a_.y) / (b_.x - a_.x) + p.x;
    }

    // setup q
    vec2 q(x, y);

    vec2 qp = glm::normalize(q - p);

    // scale by the height of the equilateral triangle
    // float h = sqrt(3.0) * 0.4 * glm::length(b_ - a_);
    float h = 0.0;

    qp = flip * h * qp;

    // get the position vector of this midpoint
    vec2 m = p + qp;

    // check if 'm' is valid
    if ( !(m.x < 0 || m.x > width - 1 || m.y < 0 || m.y > height - 1) )
      zigzags.push_back(m);

    zigzags.push_back(b_);

    a_ = b_; // update
    alpha += increment; // update lerp coefficient
  }

  if (zigzags[zigzags.size() - 1] != b)
    zigzags.push_back(b);

  // join line segments
  return zigzags;
}

std::vector<edge> Image::clipJumps(const std::vector<vec2> &normals,
                                   const std::vector<edge> &graph,
                                   float(*cost)(vec2 &, vec2 &, vec2 &)) {

  std::vector<edge> jumps;

  for (int i = 0; i < graph.size(); ++i) {
    edge e = graph[i];

    if ( isJump(e.u, e.v, normals[std::floor(e.u.y) * width + std::floor(e.u.x)],
                width, height) ) {
      // plan a new path from u to v

      jumps.push_back(e);

      /*
      // make a zig-zagger from u to v

      std::vector<vec2> zigzags = zigZag(e.u, e.v);

      // piece together to form edges
      std::vector<edge> zigzag;

      for (int i = 0; i < zigzags.size() - 1; ++i) {
        edge e(zigzags[i], zigzags[i + 1]);
        zigzag.push_back(e);
      }

      // add
      newGraph.insert(newGraph.end(), zigzag.begin(), zigzag.end());
      */

    }
    // else
      // newGraph.push_back(e);
  }

  return jumps;
}

// return all the bad stitches
std::vector<edge> Image::findJumps(const std::vector<vec2> &normals,
                                   const std::vector<edge> &graph,
                                   float(*cost)(vec2 &, vec2 &, vec2 &)) {

   std::vector<edge> jumps;

   for (int i = 0; i < graph.size(); ++i) {

     edge e = graph[i];

     if ( isJump(e.u, e.v, normals[std::floor(e.u.y) * width + std::floor(e.u.x)],
                 width, height) )
       jumps.push_back(e);
   }

   return jumps;
}

void Image::reduceNoise() {

  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {

      pixel current = getpixel(h, w);

      if (!(current.r == 128 && current.g == 128))
        setpixel(h, w, pixel(current.r, current.g, 128, 255));
    }
  }
}

// convert to adj list
void Image::convertToAdj(std::unordered_map<vec2, std::list<vec2>, HashVec> &adj,
                         const std::vector<edge> &graph) {

  for (int i = 0; i < graph.size(); ++i) {
    edge e = graph[i];

    adj[e.u].push_back(e.v);
    adj[e.v].push_back(e.u);
  }
}

// pushes new node into 'nodes'
// normals are the normals at that pixel
// search for the best node from index [start...end]
// where end is just the last index of the nodes array
// width = width of the image
void pushNode(std::vector<vec2> &nodes,
              std::vector<vec2> &normals,
              int start,
              int width,
              int height,
              float(*cost)(vec2&, vec2&, vec2 &)) {

    // get the start node
    vec2 u = nodes[start];

    u.y = std::floor(u.y);
    u.x = std::floor(u.x);

    // get the corresponding normal at 'u'
    vec2 n = normals[u.y * width + u.x];

    // convert to diff system
    u.x -= width/2;
    u.y = height/2 - u.y;

    float smallest = std::numeric_limits<float>::max();
    int nodeIndex = -1;  // index of the best node

    for (int i = start + 1; i < nodes.size(); ++i) {
        // next node, potential candidate
        vec2 v = nodes[i];
        v.x -= width/2;
        v.y = height/2 - v.y;

        // compute cost and compare
        float val = cost(u, v, n);

        if (val <= smallest) {
          // update
          nodeIndex = i;
          smallest = val;
        }
    }

    // swap
    std::swap(nodes[start + 1], nodes[nodeIndex]);
}

// new stitch plan approach
void Image::plan(std::vector<edge> &graph,
                 std::vector<vec2> &nodes,
                 std::vector<vec2> &normals,
                 float(*cost)(vec2 &, vec2 &, vec2 &)) {

 /*
 // store all the computed edges of the graph into a vector
 std::vector<vec2> nodes;
 */

 /*
 // load all pixels into the vector that are black in color
 for (int h = 0; h < height; ++h) {
     for (int w = 0; w < width; ++w) {
       pixel current = getpixel(h, w);
       if (current.r == 0 && current.g == 0 && current.b == 0)
           nodes.push_back(vec2(w, h));
     }
 }
 */

 // shuffle nodes
 std::random_shuffle(nodes.begin(), nodes.end());

 // re-arrange the nodes to get the desired order of edges
 for (int i = 0; i < nodes.size() - 1; ++i)
    pushNode(nodes, normals, i, width, height, cost);

 // store all the edges of the graph
 for (int i = 0; i < nodes.size() - 1; ++i) {
   edge e = {nodes[i], nodes[i + 1]};
   graph.push_back(e);
 }

}

void Image::replace() {

  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {
        setpixel(h, w, pixel(128, 128, 255, 255));
    }
  }
}

// gen points on a bigger grid and jitter
// nXn num samples/pixel of the og image
std::vector<vec2> Image::genPoints(std::vector<unsigned char> &densityPoints,
const int SUBGRID_SIZE) {

  std::vector<vec2> points;

  // iterate over the outer 10 X 10 grid, in the case where we have a
  // 100 X 100 image
  for (int h = 0; h < height / SUBGRID_SIZE; ++h) {
    for (int w = 0; w < width / SUBGRID_SIZE; ++w) {

      // iterate over the actual region/patch

      // move to the top left corner of the region
      int oh = h * SUBGRID_SIZE;
      int ow = w * SUBGRID_SIZE;

      // compute avg intensity of the pixels in this region
      // to find out how to subdivide the region

      float totalIntensity = 0.0;

      for (int hi = oh; hi < oh + SUBGRID_SIZE; ++hi) {
        for (int wi = ow; wi < ow + SUBGRID_SIZE; ++wi) {
          // count
          pixel current = getpixel(hi, wi);
          totalIntensity += current.r;
        }
      }

      totalIntensity /= (SUBGRID_SIZE * SUBGRID_SIZE);

      // if (totalIntensity > 230) continue;

      // sub-grid inflation constant
      const float INFLATE = 1.5;

      // use this intensity to compute sub-division size
      // with a simple linear fall-off

      // int ss = std::ceil(INFLATE * (255.0 - totalIntensity) *
                                   // SUBGRID_SIZE / 255.0);

      int ss = std::ceil(sqrt(INFLATE * SUBGRID_SIZE * SUBGRID_SIZE *
                         (255.0 - totalIntensity) / 255.0));
      // map this ss X ss grid inside the region and jitter points
      // generate ss X ss random points inside the region

      // sub-region size
      float sss = SUBGRID_SIZE / (float)ss; // ran out of good names long back

      for (int hs = 0; hs < ss; ++hs) {
        for (int ws = 0; ws < ss; ++ws) {
          // get the actual start pixel values
          float hh = oh + hs * sss;
          float ww = ow + ws * sss;

          // perturb
          hh += genRand(0.0, sss);
          ww += genRand(0.0, sss);

          // store
          points.push_back(vec2(ww, hh));
          // also store info about density of this point
          densityPoints.push_back(totalIntensity);
        }
      }
    }
  }

  return points;
}

std::vector<vec2> Image::generateNeighbors(
  const vec2 &current,
  std::unordered_map<vec2, std::vector<vec2>, HashVec> &buckets,
  const std::unordered_set<vec2, HashVec> &visited,
  const float radius,
  const int BUCKET_SIZE,
  const std::vector<edge> &segments
) {

  std::vector<vec2> neighbors;

  // find out the bucket 'current' is part of and search in its neighborhood

  // get bin co-ordinates
  float x = std::floor(current.x / (float)BUCKET_SIZE);
  float y = std::floor(current.y / (float)BUCKET_SIZE);

  // generate bin indices to get vertices in the neighborhood of 'bin'
  // the neighborhood is just 3X3 around the 'current' vertex
  for (int w = x - 1; w <= x + 1; ++w) {
    for (int h = y - 1; h <= y + 1; ++h) {
      // get the current bin index and check validity
      vec2 bin(w, h);

      if (buckets.find(bin) == buckets.end()) continue;

      // scan all nodes
      for (auto& vertex : buckets[bin]) {
        // check if node is within the radius
        if (current != vertex &&
            visited.find(vertex) == visited.end() &&
            glm::length(current - vertex) <= radius &&
            glm::length(current - vertex) >= 1.0)
          neighbors.push_back(vertex);
      }
    }
  }

  std::vector<vec2> nn;

  // filter out (current, neighbors[i]) if it intersects with the given segment
  for (int i = 0; i < neighbors.size(); ++i) {
    // test for every segment
    bool doesIntersect = false;

    for (auto& segment : segments) {
      if (doIntersect(segment.u, segment.v, current, neighbors[i])) {
        doesIntersect = true;
        break;
      }
    }
    if (!doesIntersect) nn.push_back(neighbors[i]);
  }

  return nn;
}

std::unordered_map<vec2, std::vector<vec2>, HashVec> Image::genGraph(
  const vec2 &start,
  std::unordered_map<vec2, std::vector<vec2>, HashVec> &buckets,
  const int BUCKET_SIZE,
  const std::vector<vec2> &normals,
  float(*cost)(vec2 &, vec2 &, vec2 &)
) {

  // adj list to return
  std::unordered_map<vec2, std::vector<vec2>, HashVec> graph;

  // priority queue!
  std::priority_queue<VecPair, std::vector<VecPair>, VecComparator> key;

  // vertices that have been included in the graph so far
  std::unordered_set<vec2, HashVec> visited;

  // throwaway
  std::vector<edge> segments;

  // push source into queue
  key.push(VecPair(start, 0));
  visited.insert(start);

  // radius of neighborhood
  const float radius = 5.0;

  while (!key.empty()) {

    // get the best vertex quickly!
    VecPair currentPair = key.top();
    key.pop();

    vec2 current = currentPair.vertex;
    float c = currentPair.c; // current cost

    // explore neighbors around this vertex
    // these are neighbors of current
    for (auto& neighbor : generateNeighbors(current, buckets,
                                            std::unordered_set<vec2, HashVec>{},
                                            radius, BUCKET_SIZE,
                                            segments)) {

      // get the cost of the neighbor from current
      float w = weight(current, neighbor, width, height, normals, cost);

      // check if neighbor belongs to visited
      if (visited.find(neighbor) == visited.end()) { // add to queue and form edge
        // add to the cost of current
        key.push(VecPair(neighbor, c + w));
        // add to visited
        visited.insert(neighbor);
      }

      // form edge for an undirected graph
      graph[current].push_back(neighbor);
      graph[neighbor].push_back(current);
    }
  }

  return graph;
}

vec2 Image::getSafeNeighbor(
int ch,
int cw,
int length,
const std::vector<vec2> &normals,
float(*cost)(vec2 &, vec2 &, vec2 &),
const std::vector<bool> &visited) {

  std::vector<vec2> neighbors;

  // consider only neighbors that haven't been used before

  for (int i = ch - length; i <= ch + length; ++i) {
    for (int j = cw - length; j <= cw + length; ++j) {
      if (0 < i && i < height - 1 &&
          0 < j && j < width - 1 &&
          getpixel(i, j).r == 0 &&
          !(i == ch && j == cw) &&
          !visited[i * width + j])
        neighbors.push_back(vec2(j, i));
    }
  }

  if (neighbors.size() == 0)
    return vec2(-1.0, -1.0);

  std::sort(neighbors.begin(), neighbors.end(),
           [ch, cw, this, cost, normals](vec2 a, vec2 b) -> bool {

      vec2 n = normals[ch * width + cw];
      vec2 u = vec2(cw, ch);

      // convert to the right format
      u.x -= width/2;
      u.y = height/2 - u.y;

      // also convert a and b to the right format
      a.x -= width/2;
      a.y = height/2 - a.y;

      b.x -= width/2;
      b.y = height/2 - b.y;

      return cost(u, a, n) < cost(u, b, n);

  });

  return neighbors[0];
}

// count = number of times to visit a vertex (at least)
std::vector<edge> Image::planWithDensity(
const std::vector<vec2> &normals,
int count,
float(*cost)(vec2 &, vec2 &, vec2 &)) {

  int numBlacks = 0;

  // load all pixels into the vector that are black in color
  for (int h = 0; h < height; ++h) {
      for (int w = 0; w < width; ++w) {
        pixel current = getpixel(h, w);
        if (current.r == 0 && current.g == 0 && current.b == 0)
            numBlacks++;
      }
  }

  // store all the computed nodes of the graph into a vector
  std::vector<vec2> nodes;

  // set the count
  int totalCount = count * numBlacks;

  // start
  vec2 current(1, 50);

  // keep track of the num of times a vertex has been visited
  std::vector<int> countVisits(width * height, 0);

  // visited track
  std::vector<bool> visited(width * height, false);

  /*
  while (totalCount > 0) {
    // increment count
    int c = countVisits[current.y * width + current.x]++;
    // add to nodes
    nodes.push_back(current);
    // get best neighbor for current and update
    current = getSafeNeighbor(current.y, current.x, 1, c, normals, cost);

    totalCount--;
  }
  */

  do {
    // add to nodes
    nodes.push_back(current);
    // add to visited
    visited[current.y * width + current.x] = true;
    // get best neighbor for current and update
    current = getSafeNeighbor(current.y, current.x, 2, normals, cost, visited);
  }
  while (current.x != -1.0);

  std::vector<edge> graph;
  // store all the edges of the graph
  for (int i = 0; i < nodes.size() - 1; ++i) {
    edge e = {nodes[i], nodes[i + 1]};
    graph.push_back(e);
  }

  return graph;
}

std::vector<vec2> Image::interpretNormalMap() {

  std::vector<vec2> normals;

  // loop over all pixels to get direction values
  for (int h = 0; h < height; ++h) {
      for (int w = 0; w < width; ++w) {

        pixel current = getpixel(h, w);

        unsigned char r = current.r;
        unsigned char g = current.g;
        unsigned char b = current.b;

        // convert r and g to appropriate range
        float rm = (2 * r) / 255.0 - 1;
        float rg = (2 * g) / 255.0 - 1;
        /*
        rm *= 10;
        rg *= 10;
        edge e;
        e.u = vec2(w, height - h);
        e.v = vec2(w + rm, height - h + rg);
        normals.push_back(e);
        */

        vec2 n(rm, rg);

        // generate a random vector!
        float theta = genRand(0.0, 2.0 * PI);
        vec2 p(cos(theta), sin(theta));

        /*
        if (b == 255) {
          std::vector<vec2> perpNorms{vec2(1, 0), vec2(0, 1), vec2(-1, 0), vec2(0, -1)};
          // n = perpNorms[genRand(0, perpNorms.size() - 1)];
          n.x = n.y = 0.0;
        }
        */
       
        // find the co-efficient of perturbation based on the blue channel
        // float c = (float)(b - 128.0) / 128.0;
        float c = 0.0;

        if (n.x == 0.0 && n.y == 0.0)
          normals.push_back(n);
        else
          normals.push_back(c * glm::normalize(p) +
                           (1 - c) * glm::normalize(n));
      }
  }

  return normals;
}

void Image::changeNormals(const std::vector<vec2> &points,
                          std::vector<vec2> &normals) {

  // a set of normals that the blue pixel points can pick
  std::vector<vec2> perpNorms{vec2(1, 0), vec2(0, 1), vec2(-1, 0), vec2(0, -1)};

  for (auto& point : points) {
    // get the normal pixel
    pixel normal = getpixel(std::floor(point.y),
                            std::floor(point.x));

    if (normal.b == 255) {
      // change the normal
      vec2 &n = normals[std::floor(point.y) * width + std::floor(point.x)];
      n = perpNorms[genRand(0, perpNorms.size() - 1)];
    }
  }
}

void Image::reverseNormalMap(std::vector<edge> &graph,
                             const std::vector<vec2> &normals) {

  // new image to write to
  Image *result = new Image(width, height, 4);
  result->init();

  for (int i = 0; i < graph.size(); ++i) {
    edge e = graph[i];
    vec2 current = e.u;

    // get the normal pixel
    pixel normal = getpixel(std::floor(current.y),
                            std::floor(current.x));

    // get the corresponding normal at 'u'
    vec2 n = normals[std::floor(current.y) * width + std::floor(current.x)];

    // convert to diff system
    e.u.x -= width/2;
    e.u.y = height/2 - e.u.y;

    e.v.x -= width/2;
    e.v.y = height/2 - e.v.y;

    vec2 e1 = glm::normalize(e.v - e.u);
    vec2 e2 = glm::normalize(n);

    float a = fabs(glm::dot(e1, e2));

    result->setpixel(current.y, current.x, pixel(a * normal.r, a * normal.g,
                                                 a * normal.b, 255));
  }

  // copy result into original normal map
  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {
      setpixel(h, w, result->getpixel(h, w));
    }
  }

  result->destroy();
  delete result;
}

void Image::flagOff(std::vector<bool> &isoff,
                    const std::vector<edge> &graph,
                    const std::vector<vec2> &normals) {
  
  for (int i = 0; i < graph.size(); ++i) 
    isoff.push_back(false);
  
  int count = 0;

  for (int i = 0; i < graph.size(); ++i) {
    edge e = graph[i];
    vec2 current = e.u;

    // get the normal pixel
    pixel normal = getpixel(std::floor(current.y),
                            std::floor(current.x));

    // get the corresponding normal at 'u'
    vec2 n = normals[std::floor(current.y) * width + std::floor(current.x)];

    // convert to diff system
    e.u.x -= width/2;
    e.u.y = height/2 - e.u.y;

    e.v.x -= width/2;
    e.v.y = height/2 - e.v.y;

    vec2 e1 = glm::normalize(e.v - e.u);
    vec2 e2 = glm::normalize(n);

    float a = fabs(glm::dot(e1, e2));

    if (a < 0.7) {isoff[i] = true; count++;}
  }

  float percent = float(count) / float(graph.size()) * 100.0;

  std::cout << "percent = " << percent << "\n";
}

// some helper routines to pick random elements from containers
template<typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator& g) {
    std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
    std::advance(start, dis(g));
    return start;
}

template<typename Iter>
Iter select_randomly(Iter start, Iter end) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return select_randomly(start, end, gen);
}

void Image::reverseNormalMap(std::unordered_map<vec2, std::list<vec2>, HashVec> &adj,
                             const std::vector<vec2> &normals) {

  // new image to write to
  Image *result = new Image(width, height, 4);
  result->init();

  // iterate over the adjacency list nodes
  for (auto& e : adj) {
    // current vertex
    vec2 v = e.first;

    std::list<vec2>::iterator it;
    // iterate over all neighbors

    // read in the normal at 'v'
    vec2 n = normals[std::floor(v.y) * width + std::floor(v.x)];

    // do a little conversion
    vec2 b = v;

    b.x -= width/2;
    b.y = height/2 - b.y;

    // average stitch direction vectors
    // pick a random neighbor that acts as a reference
    vec2 r = *select_randomly(e.second.begin(), e.second.end());
    // conversion for r, painful but necessary
    r.x -= width/2;
    r.y = height/2 - r.y;

    vec2 avg = r - b;

    // add all stitch direction vectors
    for (it = e.second.begin(); it != e.second.end(); ++it) {

      vec2 a = *it;

      if (a == r) continue;  // ignore the reference vector

      // need a basis conversion
      a.x -= width/2;
      a.y = height/2 - a.y;

      // get the actual vectors
      vec2 v1 = a - b;
      vec2 v2 = r - b;

      // convert a if needed
      if (glm::dot(v1, v2) < 0)
        v1 = -v1;

      // add
      avg += v1;
    }

    // read previous color and update
    pixel normal = getpixel(v.y, v.x);
    // compute dot product
    float d = 1;

    if (normal.b < 130)
      d = fabs(glm::dot(glm::normalize(avg), glm::normalize(n)));

    result->setpixel(v.y, v.x, pixel(d * normal.r, d * normal.g,
                                     d * normal.b, 255));
  }

  // copy result into original normal map
  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {
      setpixel(h, w, result->getpixel(h, w));
    }
  }

  result->destroy();
  delete result;
}

void Image::keepBlue() {

  for (int h = 0; h < height; ++h) {
      for (int w = 0; w < width; ++w) {
        pixel current = getpixel(h, w);
        if (current.b != 255)
          setpixel(h, w, pixel(255, 255, 255, 255));
        else
          setpixel(h, w, pixel(0, 0, 0, 255));
      }
  }

}

vec2 Image::chooseRandom() {

  // choose random black pixel from image and return location

  vec2 current;

  do {
    current.x = genRand(0, width - 1);
    current.y = genRand(0, height - 1);
  }
  while (getpixel(current.y, current.x).r != 0);

  return current;
}

// return the count of all the black pixels
int Image::countBlacks() {

  int count = 0;

  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {
      pixel current = getpixel(h, w);

      // printf("%d, %d, %d\n", current.r, current.g, current.b);

      if (current.r == 255 && current.g == 255 && current.b == 255)
        count++;
    }
  }

  return count;
}

// randomize a portion of the normal map
void Image::randomizeMap() {

  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {

      pixel current = getpixel(h, w);

      /*
      setpixel(h, w, pixel(genRand(0, 255),
                           genRand(0, 255),
                           genRand(0, 255),
                           255));
      */
      setpixel(h, w, pixel(128, 128, 255, 255));
    }
  }

}

// assuming a square image
void Image::sample(int size) {

  /*
  int distance = size / 2;

  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {

      // set all neighbors to be white
      pixel current = getpixel(h, w);

      if (current.r == 0) {
      for (int i = h - distance; i <= h + distance; ++i) {
        for (int j = w - distance; j <= w + distance; ++j) {
            // do not sample yourself
            if (0 <= i && i < height &&
                0 <= j && j < width &&
                getpixel(i, j).r == 0 &&
                !(i == h && j == w))
              setpixel(i, j, pixel(255, 255, 255, 255));
          }
        }
      }
    }
  }
  */

  /*
  int distance = 1;

  while (size > 0) {

    // generate a random pixel location
    int w = genRand(0, width - 1);
    int h = genRand(0, height - 1);

    if (getpixel(h, w).r == 0) {

      for (int i = h - distance; i <= h + distance; ++i) {
        for (int j = w - distance; j <= w + distance; ++j) {
            // do not sample yourself
            if (0 <= i && i < height &&
                0 <= j && j < width &&
                getpixel(i, j).r == 0 &&
                !(i == h && j == w))
              setpixel(i, j, pixel(255, 255, 255, 255));
          }
        }
      }

      size--;
    }
    */

    // a grayscale image has the same values for every channel
    for (int h = 0; h < height; ++h) {
      for (int w = 0; w < width; ++w) {

        pixel current = getpixel(h, w);

        int increment = (255 - current.r) * 0.08;

        int red = current.r + increment;
        int green = current.g + increment;
        int blue = current.b + increment;

        // clamp if needed
        if (red > 255) red = 255;
        if (green > 255) green = 255;
        if (blue > 255) blue = 255;

        setpixel(h, w, pixel(red, green, blue, 255));
      }
    }
}

// (*this) -> source
// randPic -> dest
// i, j -> start cell
// k -> partition size
// percent -> percent of values that are needed from the sub-matrix
// totalCount -> total count of black cells in (*this)
void Image::genRandNodes(Image *randPic, int i, int j, int k,
                         int percent, int &totalCount) {

  // define the horizontal range
 	int w1 = j;
 	int w2 = j + k - 1;
 	// define the vertical range
 	int h1 = i;
 	int h2 = i + k - 1;

 	// count 1s in the sub matrix of randPic[i][j] - randPic[i+k-1][j+k-1]
 	int count = 0;
 	for (int p = i; p < i + k; ++p) {
 		for (int q = j; q < j + k; ++q) {
      pixel current = getpixel(p, q);

      // pick out the black nodes
 			if (current.r == 0 && current.g == 0 && current.b == 0)
 				count++;
 		}
 	}

 	// choose some percent of values from the sub-matrix randomly now
 	double fraction = percent / 100.0;
 	int pick = ceil(fraction * count);

 	// keep picking random numbers until the quota is fulfilled
 	while (pick) {
 		// generate a random index from the ver and hor ranges
 		int w = genRand(w1, w2);
 		int h = genRand(h1, h2);

    // to get the current pixel
    pixel current = getpixel(h, w);

 		if (current.r == 0 && current.g == 0 && current.b == 0) {
      // set and unset corresponding values in the 2 images
      setpixel(h, w, pixel(255, 255, 255, 255));
      totalCount--;
      randPic->setpixel(h, w, pixel(0, 0, 0, 255));
 			// update pick and totalCount
 			pick--;
 		}
 	}
}

// return a list of neighbors that are 'length' pixels away
// from current pixel
void Image::fillNeighbors(int ch, int cw, int length,
                          std::vector<vec2> &neighbors) {

  // top row!
  int i = ch - length;
  int j;

  for (j = cw - length; j <= cw + length; ++j) {
    if (0 <= i && i < height &&
        0 <= j && j < width &&
        getpixel(i, j).r == 0)
        neighbors.push_back(vec2(j, i));
  }

  // bottom row
  i = ch + length;

  for (j = cw - length; j <= cw + length; ++j) {
    if (0 <= i && i < height &&
        0 <= j && j < width &&
        getpixel(i, j).r == 0)
        neighbors.push_back(vec2(j, i));
  }

  // first column
  j = cw - length;

  for (i = ch - length + 1; i <= ch + length - 1; ++i) {
    if (0 <= i && i < height &&
        0 <= j && j < width &&
        getpixel(i, j).r == 0)
        neighbors.push_back(vec2(j, i));
  }

  // last column
  j = cw + length;

  for (i = ch - length + 1; i <= ch + length - 1; ++i) {
    if (0 <= i && i < height &&
        0 <= j && j < width &&
        getpixel(i, j).r == 0)
        neighbors.push_back(vec2(j, i));
  }
}

// c >= 0.5
void Image::blend(const float c) {

  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {

      pixel current = getpixel(h, w);

      float b = 255.0 * c;

      setpixel(h, w, pixel(current.r,
                           current.g,
                           (unsigned char)b,
                           255));
    }
  }
}

// get stitch count ratio of horizontal to vertical stitches
float Image::SCR(const std::vector<edge> &graph) {

  // horizontal stitch to compare with
  vec2 horizontal(1.0, 0.0);

  // keep track of count for every horizontal edge
  size_t hcount = 0;
  size_t nstitches = graph.size();

  for (int i = 0; i < nstitches; ++i) {
    // get current edge direction
    edge e = graph[i];

    float closeness = fabs(glm::dot(glm::normalize(e.v - e.u), horizontal));
    // 45 degree threshold
    if (closeness > 0.7) hcount++;
  }

  std::cout << "nstitches = " << nstitches << "\n";
  std::cout << "hcount = " << hcount << "\n";

  // ratio = stitches that don't align / stitches that align
  float rat = (nstitches - hcount) / (float)hcount;

  return rat;
}

std::vector<vec2> Image::generateNeighbors(vec2& pixel, int ex, int ey) {

  // list of vertices that are neighbors of pixel
  std::vector<vec2> neighbors;

  // gen neighbors
  int cw = pixel.x;
  int ch = pixel.y;

  // search along only the diagonals with length = 1
  // good enough for high density regions!

  fillNeighbors(ch, cw, 3, neighbors);

  if (ex == -1 && ey == -1) {  // no goal

    int n = 4;
    // works coz our image is a square matrix
    while (neighbors.size() < 10 && n < width) {
      // search in a sub-matrix of size 2n X 2n (a slightly different neighborhood)
      // if neighbors is zero
      // super useful when dealing with less dense regions
      fillNeighbors(ch, cw, n, neighbors);
      // increase range
      n++;
    }
  }
  else // 4 X 4
    fillNeighbors(ch, cw, 4, neighbors);

  std::vector<vec2> closestNeighbors;

  if (ex >= 0.0 && ey >= 0.0) {
    // goal
    vec2 goal(ex, ey);

    // we have a goal, pick the 4 nearest neighbors to the goal
    if (neighbors.size() > 4) {

      for (int k = 0; k < 4; ++k)
        closestNeighbors.push_back(neighbors[k]);

      for (int s = 4; s < neighbors.size(); ++s) {
        // do we add this guy into the closest list?
        float distance = glm::length(goal - neighbors[s]);

        // find out the node amongst the 4 which is furthest away
        float maxDistance = std::numeric_limits<float>::min();
        int index = -1;

        for (int k = 0; k < 4; ++k) {
          // update index and distance if need be
          if (maxDistance < glm::length(goal - closestNeighbors[k])) {

            maxDistance = glm::length(goal - closestNeighbors[k]);
            index = k;
          }
        }

        if (distance < maxDistance) // replace
          closestNeighbors[index] = neighbors[s];
      }

      return closestNeighbors;
    }
  }

  return neighbors;
}

void preorder(vec2 &current,
              std::unordered_map<vec2, std::vector<vec2>, HashVec> &graph,
              std::unordered_set<vec2, HashVec> &seen,
              std::vector<vec2> &path,
              int& leafCount) {

  // visited!
  seen.insert(current);

  // add to path as well
  path.push_back(current);

  // get neighbors
  for (auto& neighbor : graph[current]) {
    // go down if not visited
    if (seen.find(neighbor) == seen.end())
      preorder(neighbor, graph, seen, path, leafCount);
  }

  if (graph[current].size() == 1)
    leafCount++;
}

// no need for a visited set here, since we have a tree
// no back edges, yayyy!!
// just having a parent for that node will suffice
// will also help in making zig-zags while backtracking
void Image::genPathUtil(vec2 current, vec2 parent,
std::unordered_map<vec2, std::list<vec2>, HashVec> &adj,
std::unordered_map<vec2, unsigned char, HashVec> &densities,
std::vector<vec2> &path) {

  // neighbor
  std::list<vec2> neighbors = adj[current];

  std::list<vec2>::iterator it;

  for (it = neighbors.begin(); it != neighbors.end(); ++it) {
    vec2 neighbor = *it;

    if (neighbor != parent) {
      // make zig-zag from current to neighbor
      std::vector<vec2> z = zigZag(current, neighbor, densities[current],
                                   densities[neighbor], 1.2, -1);
      // concat to the path
      // path.insert(path.end(), z.begin(), z.end());
      path.push_back(current);
      path.push_back(neighbor);

      // recurse
      genPathUtil(neighbor, current, adj, densities, path);
    }
  }

  // check if we are at the start node
  if (parent != vec2(-1, -1)) {
    // make a zig-zag in reverse now
    std::vector<vec2> z = zigZag(current, parent, densities[current],
                                 densities[parent], 1.2);
    // concat
    // path.insert(path.end(), z.begin(), z.end());
    path.push_back(current);
    path.push_back(parent);
  }
}

void Image::genPath(std::unordered_map<vec2, std::list<vec2>, HashVec> &adj,
                    std::unordered_map<vec2, unsigned char, HashVec> &densities,
                    std::vector<vec2> &path) {

  // select a random start for the traversal
  vec2 start = std::next(std::begin(adj), genRand(0, adj.size() - 1))->first;

  genPathUtil(start, vec2(-1, -1), adj, densities, path);
}

// add stitch (u, v) to an appropriate bucket in stitch buckets
void addToBucket(const vec2 &u, const vec2 &v,
std::unordered_map<vec2, std::vector<edge>, HashVec> &stitchBuckets) {

  const int SUBREGION_SIZE = 5;

  // use the mid-point of (u, v) to find an appropriate bucket
  vec2 m = 0.5f * (u + v);

  // get bin co-ordinates
  float x = std::floor(m.x / (float)SUBREGION_SIZE);
  float y = std::floor(m.y / (float)SUBREGION_SIZE);

  // add
  stitchBuckets[vec2(x, y)].push_back(edge(u, v));
}

// check if e1 and e2 are too close or not
bool isTooCloseToStitch(const edge& e1, const edge& e2, const float distance) {

  // get the endpoints of both edges
  vec2 u1 = e1.u;
  vec2 v1 = e1.v;
  vec2 u2 = e2.u;
  vec2 v2 = e2.v;

  // compare all pairs to see which one is closest
  float a1 = glm::length(u2 - u1); // u1, u2
  float a2 = glm::length(v2 - u1); // v2, u1
  float a3 = glm::length(u2 - v1); // u2, v1
  float a4 = glm::length(v2 - v1); // v2, v1

  float a = std::max(std::max(a1, a2), std::max(a3, a4));

  // closer endpoints
  edge pq;

  if (a == a1) pq = edge(u1, u2);
  else if (a == a2) pq = edge(v2, u1);
  else if (a == a3) pq = edge(u2, v1);
  else pq = edge(v2, v1);

  // pq.u, pq.v
  // we know one end-point for each stitch, time to get the second one as well
  vec2 p1, q1, p2, q2;

  p1 = pq.u;
  p2 = pq.v;

  if (p1 == u1) q1 = u2;
  else if (p1 == v2) q1 = v1;
  else q1 = u1;

  if (p2 == u2) q2 = u1;
  else if (p2 == u1) q2 = u2;
  else q2 = v2;

  // let the interpolation begin
  // generate points on both stitches and compare

  const int numPoints = 10; // the bigger, the more accurate
  float alpha = 0.0;
  const float increment = 1.0 / (float)numPoints; // 4 points per stitch

  while (alpha <= 1.0) {

    vec2 m = (1.0f - alpha) * p1 + alpha * q1;
    vec2 n = (1.0f - alpha) * p2 + alpha * q2;

    // check tolerance and reject if needed
    if (glm::length(m - n) < distance)
      return true;

    // update
    alpha += increment;
  }

  return false;
}

// is (u, v) too close to any of its neighboring stitches?
bool isTooClose(const vec2& u, const vec2& v, const float distance,
std::unordered_map<vec2, std::vector<edge>, HashVec> &stitchBuckets) {

  const int SUBREGION_SIZE = 5;

  // use the mid-point of (u, v) to get to the appropriate bucket
  vec2 m = 0.5f * (u + v);

  // get bin co-ordinates
  float x = std::floor(m.x / (float)SUBREGION_SIZE);
  float y = std::floor(m.y / (float)SUBREGION_SIZE);

  // get all the nearby stitches of (u, v)
  std::vector<edge> stitches = stitchBuckets[vec2(x, y)];

  // check if (u, v) maintains a distance 'distance' from each and every
  // one of these stitches
  for (auto& stitch : stitches) {
    if (doIntersect(u, v, stitch.u, stitch.v))
      return true;
  }

  return false;
}

std::vector<vec2> filterNeighbors(const vec2& current,
const std::vector<vec2> &neighbors,
const float distance,
std::unordered_map<vec2, std::vector<edge>, HashVec> &stitchBuckets
) {

  std::vector<vec2> goodNeighbors;

  for (const auto &neighbor : neighbors) {
    // check if neighbor is good enough or not
    if ( !isTooClose(current, neighbor, distance, stitchBuckets) )
      goodNeighbors.push_back(neighbor);
  }

  return goodNeighbors;
}

std::vector<edge> Image::dijkstra(
const vec2 &start,
std::unordered_map<vec2, std::vector<vec2>, HashVec> &buckets,
std::unordered_map<vec2, std::vector<edge>, HashVec> &stitchBuckets,
const int BUCKET_SIZE,
const std::vector<vec2> &normals,
float(*cost1)(vec2 &, vec2 &, vec2 &),
float(*cost2)(vec2 &, vec2 &, vec2 &),
const std::vector<edge>& segments
) {

   // store edges, return at the end
   std::vector<edge> edges;

   // priority queue!
   std::priority_queue<VecPair, std::vector<VecPair>, VecComparator> pq;

   // distances from source to all other vertices
   std::unordered_map<vec2, float, HashVec> distance;

   // maintain parents to make the edges
   std::unordered_map<vec2, vec2, HashVec> parent;

   // maintain all visited
   // need to do this since we have negative edge weights, yikes!
   std::unordered_set<vec2, HashVec> visited;

   // a rough distance between 2 stitches
   const float threshold = 1.0;

   // push source into queue
   pq.push(VecPair(start, 0));
   distance.insert({start, 0.0});

   const float radius = 5.0;

   // iterate until not empty
   while (!pq.empty()) {
     // get the current best vertex from the source
     VecPair currentPair = pq.top();
     pq.pop();

     vec2 current = currentPair.vertex;

     if (current != start) {
       // has to have a parent, this will the parent of current in the SPT
       vec2 p = parent[current];
       addToBucket(current, parent[current], stitchBuckets); // add this stitch to the appropriate bucket
     }

     // add to visited
     visited.insert(current);

     std::vector<vec2> neighbors = generateNeighbors(current, buckets, visited,
                                                     radius, BUCKET_SIZE,
                                                     segments);

     /*
     std::vector<vec2> gneighbors = filterNeighbors(current, neighbors,
                                                    threshold, stitchBuckets);
     */

     // filter based on proximity to other stitches
     // generate neighbors for the current vertex
     for (auto& neighbor : neighbors) {

       // float w = weight(current, neighbor, width, height, normals, cost1);

       // compute weight of the (current, neighbor) edge
       float w1 = weight(current, neighbor, width, height, normals, cost1);
       // check if current is not equal to start
       float w2 = 0;

       if (current != start)
         w2 = cost2(parent[current], current, neighbor);

       // lookup weighting parameter from the normal map
       pixel pix = getpixel(std::floor(current.y), std::floor(current.x));
       float c = (float)(pix.b - 128.0) / 128.0;

       // compute final weight
       float w = (1 - c) * w1 + c * w2;
       // float w = std::max((1-c)*w1, c*w2);

       // update distance if need be
       if (distance.find(neighbor) == distance.end() ||
           distance[neighbor] > distance[current] + w) {
         // update! the first condition implies distance of infinity
         distance[neighbor] = distance[current] + w;
         // update pq
         pq.push(VecPair(neighbor, distance[neighbor]));
         // also update the parent
         parent[neighbor] = current;
       }
     }
   }

   // iterate over all key-value pairs in the parent to form the edges
   for (auto& it : parent) {
     // generate edge and add
     edges.push_back(edge(it.first, it.second));
   }

   return edges;
}

// some abstractions to make life easy
// remove edge 'e'
void removeEdge(
const edge &e,
std::unordered_map<vec2, std::list<vec2>, HashVec> &graph) {

  graph[e.u].remove(e.v);
  graph[e.v].remove(e.u);
}

// add edge 'e'
void addEdge(
  const edge &e,
  std::unordered_map<vec2, std::list<vec2>, HashVec> &graph
) {

  graph[e.u].push_back(e.v);
  graph[e.v].push_back(e.u);
}

// eu, ev -> the edge with which there is a crossing, to be set only if
// return value is false
bool isCollisionFree(const vec2& u, const vec2& v,
vec2& eu, vec2& ev,
std::unordered_map<vec2, std::vector<edge>, HashVec> &stitchBuckets) {

  const int SUBREGION_SIZE = 5;

  // use the mid-point of (u, v) to get to the appropriate bucket
  vec2 m = 0.5f * (u + v);

  // get bin co-ordinates
  float x = std::floor(m.x / (float)SUBREGION_SIZE);
  float y = std::floor(m.y / (float)SUBREGION_SIZE);

  // get all the nearby stitches of (u, v)
  std::vector<edge> stitches = stitchBuckets[vec2(x, y)];

  // check if (u, v) maintains a distance 'distance' from each and every
  // one of these stitches
  for (auto& stitch : stitches) {
    if (doIntersect(u, v, stitch.u, stitch.v)) {
      eu = stitch.u; ev = stitch.v;
      return false;
    }
  }

  // not too close to any stitch
  return true;
}

// do not pick neighbors in 'groupNumber'
// 'v' belongs to the 'groupNumber' connected component
// we want to pick neighbors from the other group
// and then evaluate each of their fitnesses to get the best
vec2 Image::getBestNode(const vec2 &v,
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
) {

  std::vector<vec2> n = generateNeighbors(v, buckets,
                                          std::unordered_set<vec2, HashVec>{},
                                          radius, BUCKET_SIZE,
                                          segments);

  // filter out all neighbors which are in the same group
  std::vector<vec2> groupNeighbors;

  for (const auto &g : n) {
    if (group[g] != groupNumber)
      groupNeighbors.push_back(g);
  }

  // filter out all neighbors that are not collision free
  // (v, n[i]) are the edge pairs
  std::vector<vec2> neighbors;

  for (const auto &g : groupNeighbors) {
    vec2 p, q;  // formality
    if (isCollisionFree(v, g, p, q, stitchBuckets))
      neighbors.push_back(g);
  }

  // find the lowest cost neighbor and return
  float minVal = std::numeric_limits<float>::max();
  vec2 bestVertex(-1, -1);

  for (int i = 0; i < neighbors.size(); ++i) {
    vec2 neighbor = neighbors[i];

    float w = weight(v, neighbor, width, height, normals, cost);

    if (w < minVal) {
      minVal = w;
      bestVertex = neighbor;
    }
  }

  // set best cost
  bestCost = minVal;

  return bestVertex;
}

// dfs to find out the connected components for all reachable nodes
void dfs(vec2 &v,
std::unordered_map<vec2, std::list<vec2>, HashVec> &spt,
std::unordered_map<vec2, int, HashVec> &group,
int groupNumber
) {

  // set to visited
  group[v] = groupNumber;

  std::list<vec2> neighbors = spt[v];

  std::list<vec2>::iterator it;

  // iterate over all neighbors
  for (it = neighbors.begin(); it != neighbors.end(); ++it) {

    vec2 neighbor = *it;

    // recurse only if not visited
    if (group.find(neighbor) == group.end())
      dfs(neighbor, spt, group, groupNumber);
  }
}

// a more adv clip jumps procedure
// return the adj list of the new cleaned up graph
std::unordered_map<vec2, std::list<vec2>, HashVec> Image::cleanup(
  const std::vector<vec2> &normals,
  const std::vector<edge> &graph,
  float(*cost)(vec2 &, vec2 &, vec2 &),
  std::unordered_map<vec2, std::vector<vec2>, HashVec> &buckets,
  std::unordered_map<vec2, std::vector<edge>, HashVec> &stitchBuckets,
  const int BUCKET_SIZE,
  std::vector<edge> &segments) {

  // 1.) find all bad stitches
  std::vector<edge> bads = findJumps(normals, graph, cost);
  // std::vector<edge> bads = graph;

  // generate adj list from all edges in the graph
  std::unordered_map<vec2, std::list<vec2>, HashVec> spt;

  // undirected graph
  for (int i = 0; i < graph.size(); ++i) {
    edge e = graph[i];
    spt[e.u].push_back(e.v);
    spt[e.v].push_back(e.u);
  }

  const float radius = 7.0;

  // 2.) fix them...
  // update spt after every stitch fix
  // invariant: spt is always a tree!! duh
  for (int i = 0; i < bads.size(); ++i) {
    edge e = bads[i];

    // remove (e.u, e.v)
    removeEdge(e, spt);

    // group 2 connected components created by the removal of (u, v)
    std::unordered_map<vec2, int, HashVec> group;

    // group will also act as the visited set
    dfs(e.u, spt, group, 1); // label = 1
    dfs(e.v, spt, group, 2); // label = 2

    // best costs of nodes from 1 and 2
    float c1, c2;
    // choose best node for 'u' from 2
    vec2 ub = getBestNode(e.u, normals, cost, buckets, stitchBuckets,
                          group, radius, BUCKET_SIZE, c1, 1, segments);
    // choose best node for 'v' from 1
    vec2 vb = getBestNode(e.v, normals, cost, buckets, stitchBuckets,
                          group, radius, BUCKET_SIZE, c2, 2, segments);

    // choose best
    vec2 u, v; // new (u, v) to be made

    if (c1 < c2) {
      u = e.u;
      v = ub;
    }
    else {
      u = e.v;
      v = vb;
    }

    float mf = std::numeric_limits<float>::max();
    if (c1 == mf &&
        c2 == mf) {
      u = e.u; v = e.v;
    }

    // if the new (u, v) is not better than the old one, no point brother
    float w1 = weight(u, v, width, height, normals, cost);
    float w2 = weight(e.u, e.v, width, height, normals, cost);

    if (w1 < w2) // (u, v) is good :)
      addEdge(edge(u, v), spt);
    else // damn!!!! all that wasted effort :(
      addEdge(edge(e.u, e.v), spt);
  }

  /*

  /// might use this portion later for debugging

  // form edges for the new graph
  std::vector<edge> edges;

  // we're gonna end up adding all edges twice, arghh!!
  for (auto& e : spt) {
    vec2 v = e.first;

    std::list<vec2>::iterator it;
    // iterate over all neighbors
    for (it = e.second.begin(); it != e.second.end(); ++it)
      edges.push_back(edge(v, *it));
  }

  return edges;
  */

  return spt;
}

// sx, sy -> coordinates of source vertex
std::deque<edge> Image::dijkstra(const std::vector<vec2> &normals,
                                 float(*cost)(vec2 &, vec2 &, vec2 &),
                                 int sx, int sy,
                                 int ex, int ey,
                                 bool end) {

  // store edges, return at the end
  std::deque<edge> edges;

  // set the starting vertex
  vec2 start(sx, sy);

  // priority queue!
  std::priority_queue<VecPair, std::vector<VecPair>, VecComparator> pq;

  // distances from source to all other vertices
  std::unordered_map<vec2, float, HashVec> distance;

  // maintain parents to make the edges
  std::unordered_map<vec2, vec2, HashVec> parent;

  // maintain all visited
  // need to do this since we have negative edge weights, yikes!
  std::unordered_set<vec2, HashVec> visited;

  // maintain the path in the tree with the best cost
  float best = std::numeric_limits<float>::max();
  vec2 bestVertex(-1, -1);

  // push source into queue
  pq.push(VecPair(start, 0));
  distance.insert({start, 0.0});

  // iterate until not empty
  while (!pq.empty()) {
    // get the current best vertex from the source
    VecPair currentPair = pq.top();
    pq.pop();

    vec2 current = currentPair.vertex;
    // add to visited
    visited.insert(current);

    // generate neighbors for the current vertex
    std::vector<vec2> neighbors = generateNeighbors(current, -1, -1);

    // assert (neighbors.size() != 0);

    // update neighbors
    for (int i = 0; i < neighbors.size(); ++i) {
      // get current neighbor
      vec2 neighbor = neighbors[i];

      // do something only if not visited
      if (visited.find(neighbor) == visited.end()) {
        // compute weight of the (current, neighbor) edge
        float w = weight(current, neighbor, width, height, normals, cost);
        // update distance if need be
        if (distance.find(neighbor) == distance.end() ||
            distance[neighbor] > distance[current] + w) {
          // update! the first condition implies distance of infinity
          distance[neighbor] = distance[current] + w;
          // update pq
          pq.push(VecPair(neighbor, distance[neighbor]));
          // also update the parent
          parent[neighbor] = current;
          // update best cost vertex if needed
          if (distance[neighbor] < best) {
            best = distance[neighbor];
            bestVertex = neighbor;
          }
        }
      }
    }
  }

  // generate the best path
  vec2 vertex;

  if (end)
    vertex = vec2(ex, ey);  // use the end vertex
  else
    vertex = bestVertex;

  while (vertex != start) {

    // block this pixel from being used as a vertex in subsequent dijkstra's
    setpixel((int)vertex.y, (int)vertex.x, pixel(255, 255, 255, 255));

    // get the parent
    vec2 p = parent[vertex];
    edges.push_front(edge(p, vertex));
    // update
    vertex = p;
  }

  // block the start as well
  setpixel((int)vertex.y, (int)vertex.x, pixel(255, 255, 255, 255));

  /*
  // iterate over all key-value pairs in the parent to form the edges
  for (auto& it : parent) {
    // generate edge and add
    edges.push_back(edge(it.first, it.second));
  }
  */

  /*
  // maintain adj list for graph
  std::unordered_map<vec2, std::vector<vec2>, HashVec> graph;

  // undirected graph
  for (auto& it : parent) {
    graph[it.first].push_back(it.second);
    graph[it.second].push_back(it.first);
  }

  // fill in the path
  std::vector<vec2> path;
  std::unordered_set<vec2, HashVec> seen;

  int leafCount = 0;
  preorder(start, graph, seen, path, leafCount);

  // convert path to edges
  for (int i = 0; i < path.size() - 1; ++i)
    edges.push_back(edge(path[i], path[i+1]));
  */

  return edges;
}

std::unordered_map<vec2, std::vector<vec2>, HashVec> Image::genGraph() {

  // for each black pixel/vertex compute neighbors

  // maintain adj list for graph
  std::unordered_map<vec2, std::vector<vec2>, HashVec> graph;

  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {

      // current vertex
      vec2 current(w, h);

      if (getpixel(h, w).r == 0) {
        // gen neighbors
        std::vector<vec2> neighbors;
        // get 8 neighbors, for now
        fillNeighbors(h, w, 1, neighbors);

        // create an edge between every generated neighbor
        for (int i = 0; i < neighbors.size(); ++i) {
          // model an undirected graph
          graph[current].push_back(neighbors[i]);
          // graph[neighbors[i]].push_back(current);
        }
      }

    }
  }

  return graph;

}

std::vector<edge> Image::prim(

  std::unordered_map<vec2, std::vector<vec2>, HashVec> &graph,
  const std::vector<vec2> &normals,
  float(*cost)(vec2 &, vec2 &, vec2 &)

) {

  // return mst as a graph
  std::unordered_map<vec2, std::vector<vec2>, HashVec> mstTree;

  int numVertices = graph.size();

  // maintain parents to make the edges
  std::unordered_map<vec2, vec2, HashVec> parent;

  // priority queue!
  std::priority_queue<VecPair, std::vector<VecPair>, VecComparator> key;

  // maintain best edge cost for each node
  std::unordered_map<vec2, float, HashVec> distance;

  // vertices that have been included in the MST so far
  std::unordered_set<vec2, HashVec> visited;

  // init all nodes as inf
  for (auto& it : graph)
    distance.insert({it.first, INF});

  // init
  vec2 start = graph.begin()->first;
  distance[start] = 0.0;
  key.push(VecPair(start, 0.0));

  // the MST should have 'numVertices' number of vertices
  while (!key.empty()) {

    // get the best vertex quickly!
    VecPair currentPair = key.top();
    key.pop();
    vec2 current = currentPair.vertex;

    // add to visited
    visited.insert(current);

    // update neighbors!
    for (auto& neighbor : graph[current]) {

      float w = weight(current, neighbor, width, height, normals, cost);

      if (visited.find(neighbor) == visited.end() &&
          w < distance[neighbor]) {
        // update
        parent[neighbor] = current;
        distance[neighbor] = w;
        key.push(VecPair(neighbor, w));
      }
    }
  }

  std::vector<edge> edges;

  for (auto& it : parent) {
    // generate edge and add
    edges.push_back(edge(it.first, it.second));
  }

  /*
  // construct the mst tree from the parent
  for (auto& it : parent) {
    mstTree[it.first].push_back(it.second);
    mstTree[it.second].push_back(it.first);
  }

  // fill in the path
  std::vector<vec2> path;
  std::unordered_set<vec2, HashVec> seen;

  int leafCount = 0;
  preorder(start, mstTree, seen, path, leafCount);

  printf("leaves = %d\n", leafCount);

  // convert path to edges
  for (int i = 0; i < path.size() - 1; ++i)
    edges.push_back(edge(path[i], path[i+1]));
  */

  return edges;
}
