#include "SketchProgram.h"

#define SHOW_FPS

void SketchProgram::Initialize()
{
    // Modifying iostream a little bit to make cout/cin slightly faster.
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(0);

    // Initialize sdl first
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        std::cout << "Error initializing SDL: " << SDL_GetError() << std::endl;
    }
    if (IMG_Init(IMG_INIT_PNG) == 0)
    {
        std::cout << "Could not initialize SDL image.\n" << SDL_GetError();
    }

    // Just geting this info to help sync program loop ROUGHLY with user's refresh rate.
    // Figured this was safer, but this can change if necessary (probably is, SDL_Ticks aren't accurate enough).
    if (SDL_GetCurrentDisplayMode(0, &displayConfig) != 0)
    {
        std::cout << "SDL failed to get user's main display information.\n" << SDL_GetError();
    }

    // Hard-coding these here for now. Will probably be more dynamic in the future.
    screenHeight = DEF_SCREEN_HEIGHT;
    screenWidth = DEF_SCREEN_WIDTH;

    // Parse with default texture for now
    // (TODO: get image via command line or other method)
    std::string filename;
    std::string useDefault;
    std::cout << "Would you like to use a custom zone image? (Y/N)" << std::endl;
    std::cin >> useDefault;
    if (useDefault == "Y" || useDefault == "y")
    {
        std::cout << "Enter file name for region image:" << std::endl;
        std::cin >> filename;
    }
    else
    {
        filename = "oneZone.png";
    }

    ParseZoneMap(filename);

    // Arbitrary default values.
    fieldX = 32;
    fieldY = 18;
    fieldPadding = 30;
    mainVecField = std::make_shared<VectorField>(fieldX, fieldY, fieldPadding, SDL_Color {0, 0, 0, 255});
    mainVecField->InitializeVectors(normalMapColors, 20);

    // Create window and renderer
    window = (SDL_CreateWindow(WINDOW_NAME,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        DEF_SCREEN_WIDTH,
        DEF_SCREEN_HEIGHT,
        0));
    renderer = SDL_CreateRenderer(window, -1, 0);

    CreateTextureFromPixelData(normalMapTexture, normalMapPixels[0], screenWidth, screenHeight, 3);
}

void SketchProgram::MainLoop()
{
#ifdef SHOW_FPS
    int currentFPS = 0;
    int FPSCounter = 0;
    Uint32 lastCountStartTime = SDL_GetTicks();
#endif

    int frameRateTicks = ((1.0f / (float)displayConfig.refresh_rate) * 1000) + 1;
    std::cout << frameRateTicks << std::endl;
    int thisDuration = frameRateTicks;
    Uint32 thisStartTime = SDL_GetTicks();

    isRunning = true;
    while (isRunning)
    {
        thisStartTime = SDL_GetTicks();

        PollEvents();
        Update();
        Render();

        thisDuration = SDL_GetTicks() - thisStartTime;

        if (thisDuration < frameRateTicks)
        {
            SDL_Delay(frameRateTicks - thisDuration);
            thisDuration = frameRateTicks;
        }

#ifdef SHOW_FPS
        // Handle frame counter displaying in console.
        FPSCounter++;
        if (thisStartTime >= (lastCountStartTime + 1000)) // If 1000 milliseconds have passed (1s)
        {
            lastCountStartTime = thisStartTime;
            currentFPS = FPSCounter;
            FPSCounter = 0;
            std::cout << currentFPS << "\n";
        }
#endif
    }
}

void SketchProgram::PollEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            isRunning = false;
        }
    }
}

void SketchProgram::Update()
{
    // Get mouse state, as well as check for left mouse click.
    // When click is held, edits most recently added line in our list of them.
    // If click was just pressed, adds a new line to our list.
    int mouseX, mouseY;
    SketchLine* editLine = (sketchLines.empty()) ? nullptr : sketchLines.back().get();
    if ((SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(1)) != 0)
    {
        Vector2D mousePos = Vector2D(mouseX, mouseY);

        // If mouse was just clicked, create new line at mouse position.
        if (!mouseDownLastFrame)
        {
            editLine = new SketchLine(mousePos, mousePos);
            sketchLines.push_back(std::shared_ptr<SketchLine>(editLine));

            EmplaceVoronoiPoint(editLine);
        }

        editLine->SetEndPoint(mousePos);

        // Set normal encoding of vornoi point based on angle of line
        // drawn. If space is held, override that and use default normal color.
        SDL_Color normalColor = editLine->Get_RenderColor();
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_SPACE])
        {
            Helpers::NormalMapDefaultColor(&normalColor);
        }

        voronoiPoints.back()->Set_NormalEncoding(normalColor);

        mainVecField->UpdateAll();

        mouseDownLastFrame = true;
    }
    else
    {
        // If mouse click was removed, tell line to calculate what pixels it covers.
        if (mouseDownLastFrame)
        {
            pixelsToUpdate.clear();
        }
        mouseDownLastFrame = false;
    }

    // Update all pixels that are set to be updated this frame (typically because
    // they are being editted by a newly placed VoronoiPoint)
    for (auto& pix : pixelsToUpdate)
    {
        pix->UpdatePixel();
    }

    // This loop + texture update will update every pixel every frame, use w/ caution
    /*for (int x = 0; x < screenWidth; x++)
        for (int y = 0; y < screenHeight; y++)
        {
            normalMapColors[x][y]->UpdatePixel();
        }*/

    SDL_DestroyTexture(normalMapTexture);
    CreateTextureFromPixelData(normalMapTexture, normalMapPixels[0], screenWidth, screenHeight, 3);
}

void SketchProgram::Render()
{
    // Currently, background texture is same size as window which cannot be resized.
    SDL_RenderCopy(renderer, normalMapTexture, NULL, NULL);

    /*for (auto& line : sketchLines)
    {
        line->RenderLine(renderer);
    }*/

    if (!sketchLines.empty() && mouseDownLastFrame)
    {
        sketchLines.back()->RenderLine(renderer);
    }

    // Render all voronoi points as black pixels.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    for (auto& pt : voronoiPoints)
    {
        pt->RenderPoint(renderer);
    }

    for (auto& node : createdNodes)
    {
        node.second->RenderNode(renderer);
    }

    mainVecField->Render(renderer);

    SDL_RenderPresent(renderer);
}

void SketchProgram::Quit()
{
    normalMapColors.clear();
    sketchLines.clear();
    createdNodes.clear();
    PixelRGB::DeleteContiguous2DPixmap(normalMapPixels);

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
}

void SketchProgram::ParseZoneMap(const std::string& filename)
{
    std::string path = "res/zonemaps/" + filename;

    int width, height, bytes;
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &bytes, 0);

    // Clamp screen size within typical monitor size for now, since
    // image scaling is not implemented.
    screenWidth = std::min(1920, width);
    screenHeight = std::min(1080, height);

    // Initialize specific non-sdl data necessary for sketch program function.
    // These all rely on the image size, so allocate after parsing.
    normalMapPixels = PixelRGB::CreateContiguous2DPixmap(screenHeight, screenWidth);
    normalMapColors.resize(screenWidth);
    for (int x = 0; x < screenWidth; x++)
    {
        std::vector<std::shared_ptr<DynamicColor>> temp;
        temp.reserve(screenHeight);
        voronoiZonesByPixel.reserve(screenHeight);
        for (int y = 0; y < screenHeight; y++)
        {
            DynamicColor* newCol = new DynamicColor(&normalMapPixels[y][x], Vector2D(x, y));
            temp.push_back(std::shared_ptr<DynamicColor>(newCol));
        }
        normalMapColors[x].insert(normalMapColors[x].end(), temp.begin(), temp.end());
    }

    // NOW we read raw image data, and set zone information.
    std::vector<PixelRGB*> uniqueColors;
    voronoiZonesByPixel.resize(screenWidth);
    for (int x = 0; x < screenWidth; x++)
    {
        voronoiZonesByPixel[x].resize(screenHeight);
        for (int y = 0; y < screenHeight; y++)
        {
            int index = bytes * (y * width + x);
            PixelRGB* currPixel = new PixelRGB();

            currPixel->r = static_cast<unsigned char>(pixels[index]);
            currPixel->g = static_cast<unsigned char>(pixels[index + 1]);
            currPixel->b = static_cast<unsigned char>(pixels[index + 2]);

            bool isNewColor = false;
            for (int i = 0; i < uniqueColors.size(); i++)
            {
                if (PixelRGB::Equals(currPixel, uniqueColors[i]))
                {
                    isNewColor = true;
                    normalMapColors[x][y]->SetVoronoiZone(i);
                    voronoiZonesByPixel[x][y] = i;
                    break;
                }
            }

            if (!isNewColor)
            {
                uniqueColors.push_back(currPixel);
                normalMapColors[x][y]->SetVoronoiZone((int)uniqueColors.size() - 1);
                voronoiZonesByPixel[x][y] = (int)uniqueColors.size() - 1;
            }
            else
                delete currPixel;
        }
    }

    pixelsToUpdate.clear();
    pixelsToUpdate.reserve(normalMapColors.size() * normalMapColors[0].size());
    voronoiPoints.clear();
}

void SketchProgram::CreateTextureFromPixelData(SDL_Texture*& text, void* pixels, int w, int h, int channels)
{
    SDL_Surface* surfaceTemp;
    Uint32 rmask, gmask, bmask, amask;

    int depth = channels * 8;
    int pitch = channels * w;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000 >> shift;
    gmask = 0x00ff0000 >> shift;
    bmask = 0x0000ff00 >> shift;
    amask = 0x00000000;
#else 
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0x00000000;
#endif

    surfaceTemp = SDL_CreateRGBSurfaceFrom(pixels, w, h, depth, pitch, rmask, gmask, bmask, amask);

    if (surfaceTemp == NULL)
    {
        std::cout << "Surface could not be created\n" << SDL_GetError();
    }

    text = SDL_CreateTextureFromSurface(renderer, surfaceTemp);
    SDL_FreeSurface(surfaceTemp);
}

void SketchProgram::EmplaceSketchLine(SketchLine* editLine)
{
    Vector2D midpoint = editLine->Get_Midpoint();
    int radiusPixels = editLine->Get_Magnitude() / 2.0f;
    int centerX, centerY;

    // Positions are already in world space, so casting to int will round down
    // as expected and render at correct pixels around it.
    centerX = (int)midpoint[0];
    centerY = (int)midpoint[1];

    // I didn't come up with this method of getting pixels in a circle, here's where I got it
    // https://stackoverflow.com/questions/14487322/get-all-pixel-array-inside-circle
    int minX = (centerX - radiusPixels >= 0) ? (centerX - radiusPixels) : 0;
    int minY = (centerY - radiusPixels >= 0) ? (centerY - radiusPixels) : 0;
    int maxY = (centerY + radiusPixels < screenHeight) ? (centerY + radiusPixels) : screenHeight - 1;
    int maxX = (centerX + radiusPixels < screenWidth) ? (centerX + radiusPixels) : screenWidth - 1;
    SDL_Color defColor;
    Helpers::NormalMapDefaultColor(&defColor);

    //std::cout << minX << " " << minY << " " << maxX << " " << maxY << "\n";

    for (int x = minX; x <= maxX; x++)
        for (int y = minY; y <= maxY; y++)
        {
            float pixDistSqr = Vector2D(x - centerX, y - centerY).SqrMagnitude();
            float radiusSqr = radiusPixels * radiusPixels;

            if (pixDistSqr <= radiusSqr)
            {
                float t = Helpers::InverseLerp(radiusSqr, 0.0f, pixDistSqr);
                PixelRGB pixColor = Helpers::SDLColorToPixel(editLine->Get_RenderColor());
                normalMapColors[x][y]->UpdatePixelInterp(&pixColor, t);
            }
        }

    // With updated pixel info, recreate texture
    SDL_DestroyTexture(normalMapTexture);
    CreateTextureFromPixelData(normalMapTexture, normalMapPixels[0], screenWidth, screenHeight, 3);
}

void SketchProgram::EmplaceVoronoiPoint(SketchLine* editLine)
{
    int floorPosX = editLine->Get_Origin()[0];
    int floorPosY = editLine->Get_Origin()[1];
    int zone = voronoiZonesByPixel[floorPosX][floorPosY];
    std::shared_ptr<VoronoiPoint> newPoint = std::make_shared<VoronoiPoint>(editLine->Get_Origin(), editLine->Get_RenderColor(), zone);

    std::vector<DynamicColor*> pixelsToEvaluate;
    std::vector<IntersectionNode*> redundantNodes;
    // Pixels must be updated if their min point is overrwritten.
    for (int x = 0; x < screenWidth; x++)
        for (int y = 0; y < screenHeight; y++)
        {
            DynamicColor* it = normalMapColors[x][y].get();
            if (it->TryAddMinPoint(newPoint))
            {
                pixelsToEvaluate.push_back(it);

                for (auto& node : createdNodes)
                {
                    if ((it->Get_PixelPosition() - node.second->Get_Position()).SqrMagnitude() <= 0.5)
                    {
                        redundantNodes.push_back(node.second.get());
                    }
                }
            }

        }

    // Check all pixels surrounding this one; if 3 unique min voronoi points are detected,
    // then we have found an intersection and will create a node.
    // If a pixel goes out of bounds and 2 unique point are detected, then we have an
    // intersection node along the EDGE of the screen.
    // If a pixel goes out of bounds in 2 axes AND 1 unique point is found, then we have a corner.
    std::vector<IntersectionNode*> newNodes;
    std::unordered_map<int, VoronoiPoint*> affectedPoints;
    for (auto& pix : pixelsToEvaluate)
    {
        int pixX = pix->Get_PixelPosition()[0];
        int pixY = pix->Get_PixelPosition()[1];

        //std::cout << pix->Get_PixelPosition()[0] << " " << pix->Get_PixelPosition()[1] << "\n";

        int OOBx = 0;   // Determines if we HAVE GONE out of bounds at some point thus far (sign says direction)
        int OOBy = 0;   // Same as above, for y-direction.
        std::vector<VoronoiPoint*> uniquePoints;
        Vector2D averagePos = Vector2D(pixX, pixY);
        uniquePoints.push_back(newPoint.get());
        for (int x = -1; x <= 1; x++)
        {
            int checkX = pixX + x;
            OOBx = (OOBx == 0) ? ((checkX < 0) ? -1 : (checkX >= screenWidth) ? 1 : 0) : OOBx;

            for (int y = -1; y <= 1; y++)
            {
                int checkY = pixY + y;
                OOBy = (OOBy == 0) ? ((checkY < 0) ? -1 : (checkY >= screenHeight) ? 1 : 0) : OOBy;
                
                if (uniquePoints.size() + std::abs(OOBx) + std::abs(OOBy) >= 3)
                {
                    averagePos /= (int)uniquePoints.size();

                    IntersectionNode* toAdd = new IntersectionNode(averagePos, uniquePoints);

                    // If we already created a point envelopes this area, then don't create redundant node.
                    if (toAdd == nullptr) break;

                    createdNodes.emplace(toAdd->Get_ID(), std::shared_ptr<IntersectionNode>(toAdd));
                    newNodes.push_back(toAdd);
                    for (auto& pt : uniquePoints)
                    {
                        pt->AddNode(toAdd);
                    }

                    x = 2;
                    y = 2;
                    break;
                }

                if (checkY < 0 || checkY >= screenHeight) continue;
                if (checkX < 0 || checkX >= screenWidth) continue;

                bool contains = false;
                VoronoiPoint* add = normalMapColors[checkX][checkY]->Get_MinPoint();
                affectedPoints[add->Get_ID()] = add;
                for (auto* pt : uniquePoints)
                {
                    if (pt->Get_ID() == normalMapColors[checkX][checkY]->Get_MinPoint()->Get_ID())
                    {
                        contains = true;
                        break;
                    }
                }

                if (!contains)
                {
                    averagePos += Vector2D(checkX, checkY);
                    uniquePoints.push_back(add);
                }
            }
        }
    }
    pixelsToEvaluate.clear();

    // Yikes...
    // But we want to update any pixels that have been affected by the change, which
    // will involve pixels outside of the new voronoi zone.
    for (auto& pt : affectedPoints)
    {
        for (int x = 0; x < screenWidth; x++)
        {
            for (int y = 0; y < screenHeight; y++)
            {
                if (normalMapColors[x][y]->Get_MinPoint()->Get_ID() == pt.first)
                {
                    pixelsToUpdate.push_back(std::shared_ptr<DynamicColor>(normalMapColors[x][y]));
                }
            }
        }

        // Remove redundant nodes from voronoi points affected.
        for (auto& node : redundantNodes)
        {
            pt.second->RemoveNode(node);
        }
    }

    // Remove redundant nodes from global container (should be last ref. to them, making them delete as well)
    for (auto& node : redundantNodes)
    {
        createdNodes.erase(node->Get_ID());
    }

    // Now that nodes are created, we need each pixel to know what triangle
    // created by the voronoi point and its nodes that it resides in.
    // Checks for point-triangle intersection check each iteration for each pixel.
    for (auto& pix : pixelsToUpdate)
    {
        auto evalPt = pix->Get_MinPoint();
        auto nodeList = evalPt->Get_NeighboringNodes();
        bool success = false;
        for (int i = 0; i < nodeList.size(); i++)
        {
            int next = (i + 1) % nodeList.size();

            // If true, we found the triangle this pixel resides in. Now compute its relative coordinates
            // and set its required references for rendering.
            if (Helpers::PointTriangleIntersection(pix->Get_PixelPosition(), evalPt->Get_Position(),
                nodeList[i]->Get_Position(), nodeList[next]->Get_Position()))
            {
                pix->Set_TriangulationNodes(nodeList[i], nodeList[next], evalPt->Get_Position());
                success = true;
                break;
            }
        }

        if (success) continue;

        std::cout << "Error: Pixel not overlapping any triangle at: ";
        std::cout << pix->Get_PixelPosition()[0] << " " << pix->Get_PixelPosition()[1] << "\n";
    }
    voronoiPoints.push_back(std::move(newPoint));

    // With updated pixel info, recreate texture
    SDL_DestroyTexture(normalMapTexture);
    CreateTextureFromPixelData(normalMapTexture, normalMapPixels[0], screenWidth, screenHeight, 3);
}

// ---- Getters/Setters --- //

int SketchProgram::Get_ScreenHeight()
{
    return screenHeight;
}

int SketchProgram::Get_ScreenWidth()
{
    return screenWidth;
}