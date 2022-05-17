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
    std::string filename;
    std::string userInput;
    std::cout << "Would you like to use a custom zone image? (Y/N)\n";
    std::cin >> userInput;
    if (userInput == "Y" || userInput == "y")
    {
        std::cout << "Enter file name for region image:\n";
        std::cin >> filename;
    }
    ParseZoneMap(filename);

    std::cout << "Start with an empty canvas? (Y/N)\n";
    std::cin >> userInput;

    if (userInput == "N" || userInput == "n")
    {
        std::cout << "Loading default mesh...\n";
        LoadDefaultMesh();
    }

    // Arbitrary default values.
    fieldX = 32;
    fieldY = 18;
    fieldPadding = 15;
    mainVecField = std::make_unique<VectorField>(fieldX, fieldY, fieldPadding, SDL_Color {0, 0, 0, 255});
    mainVecField->InitializeVectors(normalMapColors, 20);

    // Create window and renderer
    window = (SDL_CreateWindow(WINDOW_NAME,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        screenWidth,
        screenHeight,
        0));
    renderer = SDL_CreateRenderer(window, -1, 0);

    CreateTextureFromPixelData(normalMapTexture, normalMapPixels[0], screenWidth, screenHeight, 3);

    std::cout << "Ready to draw!\n";
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
        if (event.type == SDL_WINDOWEVENT)
        {
            // Nested so we don't call SDL_GetWindowID so often.
            if (event.window.windowID == SDL_GetWindowID(window))
            {
                isRunning = false;
            }
        }
        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_q)
            {
                debugDisplay = !debugDisplay;
            }
            
            if (event.key.keysym.sym == SDLK_s)
            {
                std::string filename;
                std::string fileExtension;

                std::cout << "Save color map file as (supports .png, .bmp, .jpg): ";
                std::cin >> filename;

                SaveColorMap(filename);
            }

            if (event.key.keysym.sym == SDLK_DELETE)
            {
                DeleteSelectedPoints();
            }

            if (event.key.keysym.sym == SDLK_f)
            {
                if (sketchLines.empty()) continue;
                
                sketchLines.back()->FlipPolarity();

                for (auto& pt : selectedPoints)
                {
                    pt.second->FlipPolarity();
                }
            }
        }
    }
}

void SketchProgram::Update()
{
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    enableDeletion = keys[SDL_SCANCODE_E];
    enablePersistentSelection = keys[SDL_SCANCODE_LSHIFT];

    // Get mouse state, as well as check for mouse buttons being held at the current frame.
    // When click is held, edits most recently added line in our list of them.
    // If click was just pressed, adds a new line to our list.
    int mouseX, mouseY;
    bool leftMouseHeld = (SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(1)) != 0;
    bool middleMouseHeld = (SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(2)) != 0;
    bool rightMouseHeld = (SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(3)) != 0;
    prevMousePos = mousePos;
    mousePos = Vector2D(mouseX, mouseY);

    if (middleMouseDownLastFrame || pointPositionsDirty)
    {
        MoveSelectedPoints();
    }
    else if (rightMouseHeld)
    {
        // Cancel prev. left mouse actions if right mouse clicked;
        if (leftMouseDownLastFrame)
            pixelsToUpdate.clear();
        
        if (!rightMouseDownLastFrame)
            initSelectionPoint = mousePos;

        selectRect.w = std::abs(initSelectionPoint[0] - mousePos[0]);
        selectRect.h = std::abs(initSelectionPoint[1] - mousePos[1]);

        selectRect.x = (initSelectionPoint[0] < mousePos[0]) ? initSelectionPoint[0] : mousePos[0];
        selectRect.y = (initSelectionPoint[1] < mousePos[1]) ? initSelectionPoint[1] : mousePos[1];

        // Refreshes the selectedPoint map, removing points not overlapped and
        // adding ones that are. There's probably a better way of doing this, though.
        for (auto& pt : voronoiPoints)
        {
            const std::shared_ptr<VoronoiPoint>& vPt = pt.second;
            if (PointRectOverlap(selectRect, vPt->Get_Position()))
            {
                vPt->Set_RenderColor(white);
                selectedPoints[vPt->Get_ID()] = std::shared_ptr<VoronoiPoint>(vPt);
            }
            else if (!enablePersistentSelection)
            {
                vPt->Set_RenderColor(black);
                selectedPoints.erase(vPt->Get_ID());
            }
        }

    }
    else
    {
        selectRect.w = selectRect.h = 0;
        selectRect.x = selectRect.y = -1000;

        if (leftMouseHeld)
        {
            if (selectedPoints.empty())
            {
                DrawSketchLine(true);
            }
            else
            {
                RecolorSelectedPoints(DrawSketchLine(false));
            }

            // Update all pixels that are set to be updated this frame (typically because
            // they are being editted by a newly placed VoronoiPoint)
            for (auto& pix : pixelsToUpdate)
            {
                pix->UpdatePixel();
            }
        }
        else
        {
            pixelsToUpdate.clear();
        }
    }

    leftMouseDownLastFrame = leftMouseHeld;
    middleMouseDownLastFrame = middleMouseHeld;
    rightMouseDownLastFrame = rightMouseHeld;

    SDL_DestroyTexture(normalMapTexture);
    CreateTextureFromPixelData(normalMapTexture, normalMapPixels[0], screenWidth, screenHeight, 3);
}

void SketchProgram::Render()
{
    // Currently, background texture is same size as window which cannot be resized.
    SDL_RenderCopy(renderer, normalMapTexture, NULL, NULL);

    if (!sketchLines.empty() && leftMouseDownLastFrame)
    {
        sketchLines.back()->RenderLine(renderer);
    }

    // Render all voronoi points as black pixels.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    for (auto& pt : voronoiPoints)
    {
        pt.second->RenderPoint(renderer);
    }

    if (debugDisplay)
    {
        for (auto& pt : voronoiPoints)
        {
            pt.second->RenderFormedTriangles(renderer);
        }

        for (auto& node : createdNodes)
        {
            node.second->RenderNode(renderer);
        }
    }

    mainVecField->Render(renderer);

    if (rightMouseDownLastFrame)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &selectRect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    }


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

void SketchProgram::SaveColorMap(std::string& filename)
{
    int errCheck;
    int texW, texH;
    int format = SDL_PIXELFORMAT_RGB24;
    
    // Get width and height of texture in case we add additional visuals around screen.
    errCheck = SDL_QueryTexture(normalMapTexture, NULL, NULL, &texW, &texH);
    if (errCheck)
    {
        std::cout << "Error querying from normal map texture\n";
        std::cout << SDL_GetError();
        return;
    }

    // normalMapPixels array is tied directly to normalMapTexture, so this works as if
    // we read the texture pixels directly instead
    SDL_Surface* pixelsToSurf = SDL_CreateRGBSurfaceWithFormatFrom(normalMapPixels[0], texW, texH, SDL_BITSPERPIXEL(format),
        texW * SDL_BYTESPERPIXEL(format), format);

    int off = filename.find_last_of('.');
    std::string fileExtension = filename.substr(off, filename.size() - off);

    if (fileExtension == ".bmp")
        errCheck = SDL_SaveBMP(pixelsToSurf, filename.c_str());
    else if (fileExtension == ".png")
        errCheck = IMG_SavePNG(pixelsToSurf, filename.c_str());
    else if (fileExtension == ".jpg")
        errCheck = IMG_SaveJPG(pixelsToSurf, filename.c_str(), 95); // may not work if sdl_image not built with libjpeg
    else
    {
        std::cout << fileExtension << " is not supported. Aborting save.\n";
        return;
    }

    if (errCheck)
    {
        std::cout << "Error saving color map pixels to bmp\n";
        std::cout << SDL_GetError();
    }

    std::cout << "Saved image to " << filename << "\n";
}

void SketchProgram::ParseZoneMap(const std::string& filename)
{
    int width, height, bytes;
    unsigned char* pixels;
    if (filename.empty())
    {
        width = DEF_SCREEN_WIDTH;
        height = DEF_SCREEN_HEIGHT;
        bytes = 3;

        long totalLen = bytes * width * height;
        pixels = new unsigned char[totalLen];
        for (long i = 0; i < totalLen; i++)
        {
            pixels[i] = 0;
        }
    }
    else
    {
        std::string path = "res/zonemaps/" + filename;
        pixels = stbi_load(path.c_str(), &width, &height, &bytes, 0);
    }

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
        std::vector<DynamicColor*> temp;
        temp.reserve(screenHeight);
        voronoiZonesByPixel.reserve(screenHeight);
        for (int y = 0; y < screenHeight; y++)
        {
            DynamicColor* newCol = new DynamicColor(&normalMapPixels[y][x], Vector2D(x, y));
            temp.push_back(newCol);
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

            bool isNewColor = true;
            for (int i = 0; i < uniqueColors.size(); i++)
            {
                if (PixelRGB::Equals(currPixel, uniqueColors[i]))
                {
                    isNewColor = false;
                    normalMapColors[x][y]->SetVoronoiZone(i);
                    voronoiZonesByPixel[x][y] = i;
                    break;
                }
            }

            if (isNewColor)
            {
                uniqueColors.push_back(currPixel);
                normalMapColors[x][y]->SetVoronoiZone((int)uniqueColors.size() - 1);
                voronoiZonesByPixel[x][y] = (int)uniqueColors.size() - 1;
                std::cout << "Test\n";
            }
            else
                delete currPixel;
        }
    }

    pixelsToUpdate.clear();
    pixelsToUpdate.reserve(normalMapColors.size() * normalMapColors[0].size());
    voronoiPoints.clear();
}

void SketchProgram::LoadDefaultMesh()
{
    int padding = screenWidth * 0.05;
    int maxX = screenWidth - padding - padding;
    int maxY = screenHeight - padding - padding;

    int sizeX = 10;
    int sizeY = 10;

    // Place points with padding and even spacing w/ same strategy as the VectorField creation
    for (int x = 0; x < sizeX; x++)
        for (int y = 0; y < sizeY; y++)
        {
            std::cout << "Spawning Point: x = " << x << ", y = " << y << "\n";
            float interpX = (float)x / (sizeX - 1);
            float interpY = (float)y / (sizeY - 1);
            Vector2D pos = Vector2D((interpX * maxX) + padding, (interpY * maxY) + padding);

            int pixX = (int)pos[0];
            int pixY = (int)pos[1];

            SDL_Color defCol;
            /*defCol.r = (Uint8)Helpers::Lerp(0, 255, interpX);
            defCol.g = (Uint8)Helpers::Lerp(0, 255, interpY);
            defCol.b = 128;
            defCol.a = 255;*/

            Helpers::NormalMapDefaultColor(&defCol);
            int zone = voronoiZonesByPixel[pixY][pixX];
            std::shared_ptr<VoronoiPoint> newPoint = std::make_shared<VoronoiPoint>(pos, defCol, zone);
            EmplaceVoronoiPoint(newPoint, false);
            newestPointID = newPoint->Get_ID();
            voronoiPoints[newestPointID] = std::move(newPoint);
        }

    for (int x = 0; x < screenWidth; x++)
    {
        BarycentricUpdate(normalMapColors[x]);
        for (int y = 0; y < screenHeight; y++)
        {
            normalMapColors[x][y]->UpdatePixel();
        }
    }

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

void SketchProgram::EmplaceVoronoiPoint(std::shared_ptr<VoronoiPoint>& newPoint, bool updateAffectedBarycentric)
{
    int floorPosX = newPoint->Get_Position()[0];
    int floorPosY = newPoint->Get_Position()[1];
    int zone = newPoint->Get_VoronoiZone();

    std::vector<DynamicColor*> pixelsToEvaluate;
    std::vector<int> redundantNodes;

    // Pixels must be updated if their min point is overrwritten.
    for (int x = 0; x < screenWidth; x++)
        for (int y = 0; y < screenHeight; y++)
        {
            if (zone != voronoiZonesByPixel[x][y]) continue;

            DynamicColor* it = normalMapColors[x][y];
            if (it->TryAddMinPoint(newPoint))
            {
                pixelsToEvaluate.push_back(it);

                for (auto& node : createdNodes)
                {
                    if ((it->Get_PixelPosition() - node.second->Get_Position()).SqrMagnitude() <= 1.5)
                    {
                        redundantNodes.push_back(node.first);
                    }
                }
            }

        }

    // Check all pixels surrounding this one; if 3 unique min voronoi points are detected,
    // then we have found an intersection and will create a node.
    // If a pixel goes out of bounds and 2 unique point are detected, then we have an
    // intersection node along the EDGE of the screen.
    // If a pixel goes out of bounds in 2 axes AND 1 unique point is found, then we have a corner.
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

                if (checkY < 0 || checkY >= screenHeight) continue;
                if (checkX < 0 || checkX >= screenWidth) continue;

                bool contains = false;
                VoronoiPoint* add = normalMapColors[checkX][checkY]->Get_MinPoint();
                if (add == nullptr) break;

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

        if (uniquePoints.size() + std::abs(OOBx) + std::abs(OOBy) >= 3)
        {
            averagePos /= (int)uniquePoints.size();

            std::shared_ptr<IntersectionNode> toAdd = std::make_shared<IntersectionNode>(averagePos, uniquePoints, zone);

            createdNodes.emplace(toAdd->Get_ID(), std::shared_ptr<IntersectionNode>(toAdd));
            for (auto& pt : uniquePoints)
            {
                pt->AddNode(std::shared_ptr<IntersectionNode>(toAdd));
            }
        }
    }
    pixelsToEvaluate.clear();

    // Yikes...
    // But we want to update any pixels that have been affected by the change, which
    // will involve pixels outside of the new voronoi zone.
    for (auto& pt : affectedPoints)
    {
        if (updateAffectedBarycentric)
        {
            for (int x = 0; x < screenWidth; x++)
            {
                for (int y = 0; y < screenHeight; y++)
                {
                    VoronoiPoint* minPt = normalMapColors[x][y]->Get_MinPoint();
                    if (minPt == nullptr) continue;

                    if (minPt->Get_ID() == pt.first && minPt->Get_VoronoiZone() == pt.second->Get_VoronoiZone())
                    {
                        pixelsToUpdate.push_back(normalMapColors[x][y]);
                    }
                }
            }
        }

        // Remove redundant nodes from voronoi points affected.
        for (auto& nodeID : redundantNodes)
        {
            pt.second->RemoveNode(nodeID);
        }
    }

    // Remove redundant nodes from global container (should be last ref. to them, making them delete as well)
    for (auto& nodeID : redundantNodes)
    {
        createdNodes.erase(nodeID);
    }

    // Now that nodes are created, we need each pixel to know what triangle
    // created by the voronoi point and its nodes that it resides in.
    // Checks for point-triangle intersection check each iteration for each pixel.
    if (updateAffectedBarycentric)
        BarycentricUpdate(pixelsToUpdate);
}

bool SketchProgram::PointRectOverlap(const SDL_Rect& aabb, const Vector2D& pt)
{
    return (aabb.x <= pt[0] && aabb.x + aabb.w >= pt[0] &&
            aabb.y <= pt[1] && aabb.y + aabb.h >= pt[1]);
}

SketchLine* SketchProgram::DrawSketchLine(bool placePoint)
{
    SketchLine* editLine = (sketchLines.empty()) ? nullptr : sketchLines.back().get();

    // If mouse was just clicked, create new line at mouse position.
    if (!leftMouseDownLastFrame)
    {
        editLine = new SketchLine(mousePos, mousePos);
        sketchLines.push_back(std::shared_ptr<SketchLine>(editLine));

        if (placePoint)
        {
            int zone = voronoiZonesByPixel[(int)mousePos[0]][(int)mousePos[1]];
            std::shared_ptr<VoronoiPoint> newPoint = std::make_shared<VoronoiPoint>(editLine->Get_Origin(), editLine->Get_RenderColor(), zone);
            EmplaceVoronoiPoint(newPoint);
            newestPointID = newPoint->Get_ID();
            voronoiPoints[newestPointID] = std::move(newPoint);
        }
    }

    editLine->SetEndPoint(mousePos);

    // Set normal encoding of vornoi point based on angle of line
    // drawn. If space is held, override that and use default normal color.
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    editLine->SetColorMode(keys[SDL_SCANCODE_SPACE]);
    editLine->UpdateColor();

    if (placePoint) 
    {
        voronoiPoints[newestPointID]->Set_NormalEncoding(editLine->Get_RenderColor());
        voronoiPoints[newestPointID]->Set_Polarity(editLine->Get_Polarity());
    }
    mainVecField->UpdateAll();

    return editLine;
}

void SketchProgram::MoveSelectedPoints()
{
    if (middleMouseDownLastFrame)
    {
        pointPositionsDirty = true;
        for (auto& vPt : selectedPoints)
        {
            vPt.second->Set_Position(vPt.second->Get_Position() - (prevMousePos - mousePos));
        }
    }
    else if (pointPositionsDirty)
    {
        RebuildMapNaive();
        pointPositionsDirty = false;
    }

}

void SketchProgram::BarycentricUpdate(const std::vector<DynamicColor*>& toUpdate)
{
    // If a triangle formed by using the voronoi point and two adjacent nodes overlaps a pixel,
    // then it resides within that triangle and should update color according to those nodes
    // and calculated coordinates.
    for (auto& pix : toUpdate)
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

        //std::cout << "Could not find triangle for pixel\n";
    }
}

void SketchProgram::CheckForIntersections(const std::vector<DynamicColor*>& toCheck)
{
    for (auto& pix : toCheck)
    {
        int pixX = pix->Get_PixelPosition()[0];
        int pixY = pix->Get_PixelPosition()[1];
        int zone = pix->Get_VoronoiZone();

        //std::cout << pix->Get_PixelPosition()[0] << " " << pix->Get_PixelPosition()[1] << "\n";

        int OOBx = 0;   // Determines if we HAVE GONE out of bounds at some point thus far (sign says direction)
        int OOBy = 0;   // Same as above, for y-direction.
        std::vector<VoronoiPoint*> uniquePoints;
        Vector2D averagePos = Vector2D(pixX, pixY);
        for (int x = -1; x <= 1; x++)
        {
            int checkX = pixX + x;
            OOBx = (OOBx == 0) ? ((checkX < 0) ? -1 : (checkX >= screenWidth) ? 1 : 0) : OOBx;

            for (int y = -1; y <= 1; y++)
            {
                int checkY = pixY + y;
                OOBy = (OOBy == 0) ? ((checkY < 0) ? -1 : (checkY >= screenHeight) ? 1 : 0) : OOBy;

                if (checkY < 0 || checkY >= screenHeight) continue;
                if (checkX < 0 || checkX >= screenWidth) continue;

                bool contains = false;
                VoronoiPoint* add = normalMapColors[checkX][checkY]->Get_MinPoint();
                if (add == nullptr) break;

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

        if (uniquePoints.size() + std::abs(OOBx) + std::abs(OOBy) >= 3)
        {
            averagePos /= (int)uniquePoints.size();

            std::shared_ptr<IntersectionNode> toAdd = std::make_shared<IntersectionNode>(averagePos, uniquePoints, zone);

            createdNodes.emplace(toAdd->Get_ID(), std::shared_ptr<IntersectionNode>(toAdd));
            for (auto& pt : uniquePoints)
            {
                pt->AddNode(std::shared_ptr<IntersectionNode>(toAdd));
            }
        }
    }
}

void SketchProgram::RebuildMapNaive()
{
    for (int x = 0; x < screenWidth; x++)
        for (int y = 0; y < screenHeight; y++)
        {
            normalMapColors[x][y]->ClearVoronoiData();
        }

    for (auto& vPt : voronoiPoints)
    {
        vPt.second->ClearNodes();
    }

    createdNodes.clear();

    std::cout << "Rebuilding map...\n";
    int cnt = voronoiPoints.size();
    int cur = 1;
    for (auto& vPt : voronoiPoints)
    {
        EmplaceVoronoiPoint(vPt.second, false);
        std::cout << "(" << cur++ << "/" <<  cnt << ") points placed\n";
    }

    // These similar loops might seem silly, but rebuilding the map is already incredibly slow,
    // so I threw this if statement at the top level like so to reduce slowdown
    // from thousands of branches
    std::cout << "Updating pixels...\n";
    if (!voronoiPoints.empty())
    {
        for (int x = 0; x < screenWidth; x++)
        {
            BarycentricUpdate(normalMapColors[x]);
            for (int y = 0; y < screenHeight; y++)
            {
                normalMapColors[x][y]->UpdatePixel();
            }
        }
    }
    else
    {
        for (int x = 0; x < screenWidth; x++)
        {
            for (int y = 0; y < screenHeight; y++)
            {
                normalMapColors[x][y]->UpdatePixel();
            }
        }
    }

    mainVecField->UpdateAll();
        
    std::cout << "Map Rebuilt!\n";

    SDL_DestroyTexture(normalMapTexture);
    CreateTextureFromPixelData(normalMapTexture, normalMapPixels[0], screenWidth, screenHeight, 3);
}

void SketchProgram::RecolorSelectedPoints(SketchLine* followLine)
{
    // Below is code that tries to optimize which pixels should update when recoloring
    // multiple points, but it doesn't work right now; for now just update all pixels.
    
    if (!leftMouseDownLastFrame)
    {
        pixelsToUpdate.clear();

        for(int x = 0; x < screenWidth; x++)
            for (int y = 0; y < screenHeight; y++)
            {
                DynamicColor* it = normalMapColors[x][y];
                if (it->Get_TriNodeA() == nullptr || it->Get_TriNodeB() == nullptr)
                    continue;

                bool found = false;
                // Pixel should update if it's barcentric blending is involved with the points'
                // nodes, since those nodes are influenced by the points themselves.
                for (auto& ptA : it->Get_TriNodeA()->Get_IntersectingPoints())
                {
                    if (selectedPoints.count(ptA->Get_ID()) >= 1)
                    {
                        pixelsToUpdate.push_back(it);
                        found = true;
                        break;
                    }
                }

                if (found) continue;    // Save some computing time if we know we found a pixel that should update already.
                
                for (auto& ptB : it->Get_TriNodeB()->Get_IntersectingPoints())
                {
                    if (selectedPoints.count(ptB->Get_ID()) >= 1)
                    {
                        pixelsToUpdate.push_back(it);
                        break;
                    }
                }
            }

    }

    SDL_Color cacheCol = followLine->Get_RenderColor();
    bool cachePolarity = followLine->Get_Polarity();
    for (auto& vPt : selectedPoints)
    {
        vPt.second->Set_NormalEncoding(cacheCol);
        vPt.second->Set_Polarity(cachePolarity);
    }

    //for (int x = 0; x < screenWidth; x++)
    //    for (int y = 0; y < screenHeight; y++)
    //    {
    //        normalMapColors[x][y]->UpdatePixel();
    //    }
}

void SketchProgram::DeleteSelectedPoints()
{
    for (auto& pt : selectedPoints)
    {
        voronoiPoints.erase(pt.first);
        for (auto& node : pt.second->Get_NeighboringNodes())
        {
            createdNodes.erase(node->Get_ID());
        }
    }

    RebuildMapNaive();
    selectedPoints.clear();
}

void SketchProgram::CreateStitchDiagram()
{
   
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