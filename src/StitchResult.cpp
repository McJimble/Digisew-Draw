#include "StitchResult.h"
#include "Helpers.h"

#include <fstream>
#include <ostream>
#include <sstream>
#include <memory>
#include <cstdio>
#include <direct.h>

int StitchResult::resultID = 0;

// These were globals in the legacy stitch code, gotta carry it over the same way
// to avoid a ton of unnecessary refactoring.
float alpha1 = 1.4f;
float alpha2 = 2.2f;
float beta1 = 2.05f;
float beta2 = 4.4f;

StitchResult::StitchResult(int w, int h, int wn, int hn, PixelRGB**& normalMap, PixelRGB**& densityMap)
{
    this->stitchImg = std::make_unique<Image>(w, h, 4);
    this->densityMapImg = std::make_unique<Image>(wn, hn, 4);
    this->normalMapImg = std::make_unique<Image>(wn, hn, 4);

    // Scale normal map up/down based on the density image provided.
    // Performs a psuedo-scaling method that results in a non-filtered version of the original.
    float incrementX = (float)w / wn;
    float incrementY = (float)h / hn;
    float xf = 0.01f;
    float yf = 0.01f;

    for (int x = 0; x < wn; ++x)
    {
        for (int y = 0; y < hn; ++y)
        {
            const PixelRGB& pix = normalMap[(int)xf][(int)yf];  // Cached for speed
            normalMapImg->setpixel(hn - y - 1, x, pixel(pix.r, pix.g, pix.b, 255));

            yf = Helpers::Clamp(yf + incrementY, 0, h - 1);
        }

        // Increment scale values
        xf = Helpers::Clamp(xf + incrementX, 0, w - 1);
        yf = 0.01f;
    }

    // Do same for density, but using reciprocal increment.
    incrementX = (float)wn / w;
    incrementY = (float)hn / h;
    xf = 0.01f;
    yf = 0.01f;
    for (int x = 0; x < w; ++x)
    {
        for (int y = 0; y < h; ++y)
        {
            const PixelRGB& denP = densityMap[y][x];
            densityMapImg->setpixel(hn - (int)yf - 1, wn - (int)xf - 1, pixel(denP.r, denP.g, denP.b, 255));

            yf = Helpers::Clamp(yf + incrementY, 0, h - 1);
        }

        // Increment scale values
        xf = Helpers::Clamp(xf + incrementX, 0, w - 1);
        yf = 0.01f;
    }
}

StitchResult::~StitchResult()
{
    if (window != nullptr)
    {
        SDL_DestroyWindow(window);
    }

    if (renderer != nullptr)
    {
        SDL_DestroyRenderer(renderer);
    }
}

float cost1(vec2& u, vec2& v, vec2& n)
{
    float distance = glm::length(v - u);
    float cosangle = 0.0;

    if (n.x != 0.0 || n.y != 0.0) {
        vec2 e1 = glm::normalize(v - u);
        vec2 e2 = glm::normalize(n);
        cosangle = fabs(glm::dot(e1, e2));
    }

    return -pow(alpha1, -distance) * pow(beta1, cosangle);
};

float cost2(vec2& u, vec2& v, vec2& w) {

    // set both the control parameters as equal
    // both length of the stitch and angle between stitches
    // have an equal weight
    float result = 0.0f;

    vec2 e1 = glm::normalize(u - v);
    vec2 e2 = glm::normalize(w - v);
    float cosangle = abs(glm::dot(e1, e2));
    float distance = glm::length(w - v);
    // float distance = abs(w.x - v.x) + abs(w.y - v.y);

    //result = -pow(alpha2, -distance) * beta1 * pow(beta2, -cosangle);
    result = -pow(alpha2, -distance) * pow(beta2, -cosangle);
    return result;
}

bool StitchResult::CreateStitches(bool createWindow)
{
    int imgWidth = stitchImg->getWidth();
    int imgHeight = stitchImg->getHeight();
    int normWidth = normalMapImg->getWidth();
    int normHeight = normalMapImg->getHeight();

    std::string fileName;
    std::cout << "Enter file name for output DST: \n";
    std::cin >> fileName;

    if (fileName.find(".dst") <= fileName.size() - 1)
        fileName = fileName.substr(0, fileName.find(".dst"));

    // Beginning of legacy stitch generation, with a few modifications for compatibility
    // ---------
    std::string dstName = fileName + ".csv";

    // store the corresponding intensity values of the points
    std::vector<unsigned char> densityPoints;

    std::vector<vec2> points = densityMapImg->genPoints(densityPoints, subgridSize);

    std::ofstream pdata("output/points.txt");

    for (int i = 0; i < points.size(); ++i) {
        vec2 point = points[i];
        pdata << point.x << " " << point.y << "\n";
    }

    pdata.close();

    // create a map for points and their corresponding densities
    std::unordered_map<vec2, unsigned char, HashVec> densities;

    // read in the no-gos
    std::vector<edge> segments;

    std::cout << "# of points = " << points.size() << "\n";

    // place points in bins/buckets
    std::unordered_map<vec2, std::vector<vec2>, HashVec> buckets;

    const int SUBREGION_SIZE = 4;

    // init the buckets, 10X10 100 buckets
    for (size_t i = 0; i < normWidth / SUBREGION_SIZE; ++i)
        for (size_t j = 0; j < normHeight / SUBREGION_SIZE; ++j)
            buckets.insert({ vec2(i, j), {} });

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
    std::ifstream inparams("res/params/inter_params.txt");

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

    std::cout << "a1 = " << alpha1 << ", b1 = " << beta1 << ", a2 = " << alpha2 << ", b2 = " << beta2 << "\n";
    std::cout << "w = " << bw << "\n";

    std::cout << "\nBuilding stitch....\n";

    // Temp copy of normal map that is later reversed in legacy code, so this
    // was required to make it work with new setup.
    std::unique_ptr<Image> reverseNormMap = std::make_unique<Image>(normWidth, normHeight, 4);
    for (int x = 0; x < normWidth; x++)
        for (int y = 0; y < normHeight; y++)
        {
            reverseNormMap->setpixel(normHeight - y - 1, x, normalMapImg->getpixel(x, y));
        }

    bw = 0.0;
    //beta2 = 5.0;
    reverseNormMap->blend(bw * 0.5 + 0.5);

    std::vector<vec2> normals = reverseNormMap->interpretNormalMap();

    // generate random start
    const vec2 start = points[genRand(0, points.size() - 1)];

    std::unordered_map<vec2, std::vector<edge>, HashVec> stitchBuckets;

    // cost1 and cost 2 funcs. below are used in image->dijkstra(), which requires
    // functions pointers not tied to any type to determine cost
    std::vector<edge> g = reverseNormMap->dijkstra(start, buckets, stitchBuckets,
        SUBREGION_SIZE,
        normals, cost1, cost2,
        segments);

    // write the dirty stitch plan to a file for a later render
    std::ofstream ddata("output/dirty.txt");

    for (int i = 0; i < g.size(); ++i) {
        edge e = g[i];
        ddata << e.u.x << " " << e.u.y << " " << e.v.x << " " << e.v.y << "\n";
    }

    ddata.close();

    reverseNormMap->reverseNormalMap(g, normals);

    // fix jumps and get final graph
    std::unordered_map<vec2, std::list<vec2>, HashVec> adj =
        reverseNormMap->cleanup(normals, g, cost1,
            buckets, stitchBuckets, SUBREGION_SIZE,
            segments);

    reverseNormMap->reverseNormalMap(adj, normals);

    // tree traversal for path generation
    std::vector<vec2> path;
    reverseNormMap->genPath(adj, densities, path);

    std::vector<edge> graph;
    // convert path to edges
    for (int i = 0; i < path.size() - 2; ++i) {

        graph.push_back(edge(path[i], path[i + 1]));
    }

    // print stitch count ratio
    float rat = Image::SCR(g);
    std::cout << "SCR = " << rat << "\n";

    std::vector<bool> isoff;
    reverseNormMap->flagOff(isoff, graph, normals);

    const float Height = 100;
    const float Width = 100;

    float xr = imgWidth / Width;
    float yr = imgWidth / Height;

    std::vector<pixel> v1 = reverseNormMap->readImageIntoBuffer();

    std::vector<pixel> v2 = normalMapImg->readImageIntoBuffer();

    pixel p = Image::RMSError(v2, v1);

    std::ofstream data(dstName);

    if (createWindow)
    {
        std::string winName = "Stitch Result ";
        winName += std::to_string(++resultID);

        window = (SDL_CreateWindow(winName.c_str(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            imgWidth,
            imgHeight,
            0));

        renderer = SDL_CreateRenderer(window, -1, 0);
    }

    // Legacy code addition; drawing with SDL lines over black background instead.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, NULL);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (int i = 0; i < graph.size(); i += 1) {
        // read consecutive points to draw edge
        float x1 = graph[i].u.x;
        float y1 = Height - graph[i].u.y;
        float x2 = graph[i].v.x;
        float y2 = Height - graph[i].v.y;

        std::ostringstream s1;
        s1 << Width - x1;
        std::string sx1(s1.str());

        std::ostringstream s2;
        s2 << Height - y1;
        std::string sy1(s2.str());

        // On first iteration, jump to first stitch position. This is what removes the
        // the "jump" stitch going across from a corner to a random stitch position.
        if (i == 0)
            data << "\"*\", \"JUMP\", " << "\"" << sx1 << "\", " << "\"" << sy1 << "\", " << "1\n";
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

        // Draw line to renderer, accounting for SDL inverting the y-axis.
        SDL_RenderDrawLineF(renderer, imgWidth - x1, y1, imgWidth - x2, y2);
        //std::cout << imgWidth - x1 << " " << y1 << " " << imgWidth - x2 << " " << y2 << "\n";
    }

    data.close();

    // Save what screen rendered to sitchImg now.
    SDL_Surface* surfaceTemp;
    Uint32 rmask, gmask, bmask, amask;

    int channels = 4;   // Reading only RGB later, but apparently renderer padds with an extra byte!
    int depth = channels * 8;
    int pitch = channels * imgWidth;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000 >> shift;
    gmask = 0x00ff0000 >> shift;
    bmask = 0x0000ff00 >> shift;
    amask = 0x000000ff;
#else 
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    surfaceTemp = SDL_CreateRGBSurface(0, imgWidth, imgHeight, depth, rmask, gmask, bmask, amask);
    if (surfaceTemp == NULL)
    {
        std::cout << "Surface could not be created\n" << SDL_GetError();
        return false;
    }

    SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGB888, surfaceTemp->pixels, surfaceTemp->pitch);
    unsigned char* pixels = (unsigned char*)surfaceTemp->pixels;
    for (int x = 0; x < imgWidth; x++)
        for (int y = 0; y < imgHeight; y++)
        {
            pixel pix;
            int index = 3 * (y * imgWidth + x);
            pix.r = pixels[index];
            pix.g = pixels[index + 1];
            pix.b = pixels[index + 2];
            pix.a = 255;
            stitchImg->setpixel(y, x, pix);
        }

    // run lib emb convert
    char command[200];
    std::string outputName = dstName.substr(0, dstName.find(".csv")) + ".dst";
    std::cout << outputName << std::endl;

    sprintf_s(command,
        "libembroidery-convert.exe %s %s",
        dstName.c_str(),
        outputName.c_str());

    // system call
    // I don't like this, but for Windows there really is no easy way to do this
    // in the same fashion as the legacy Linux stitching code. If speed/security
    // becomes a problem in the future for converting the embroidery files, this
    // needs to be changed !!
    // - James
    system(command);

    if (std::rename(dstName.c_str(), ("output/csv/" + dstName).c_str()) != 0)
    {
        std::cout << "Failed to save to: " << ("output/csv/" + dstName + "\n");
    }

    if (std::rename(outputName.c_str(), ("output/dst/" + outputName).c_str()) != 0)
    {
        std::cout << "Failed to save to: " << ("output/dst/" + outputName + "\n");
    }
    else
    {
        std::cout << "Successfully saved to: " << ("output/dst/" + outputName + "\n");
    }

    SDL_UpdateWindowSurface(window);
    SDL_RenderPresent(renderer);

    return true;
}