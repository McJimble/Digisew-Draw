#include "SketchProgram.h"

#include "stb/stb_image.h"

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

#define SHOW_FPS

void SketchProgram::Initialize(int argc, char** argv)
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

    std::string paramsName = "res/params/start_params.txt";
    if (argc >= 2)
        paramsName = std::string(argv[1]);

    ReadParameters(paramsName.c_str());

    screenWidth = pWidth;
    screenHeight = pHeight;

    ParseZoneMap(zoneMapName);

    std::string userInput;
    std::cout << "Start with default square mesh? (Y/N)\n";
    std::cin >> userInput;

    if (userInput == "Y" || userInput == "y")
    {
        std::cout << "Loading default mesh...\n";
        LoadDefaultMesh();
    }

    // Arbitrary default values.
    fieldX = 32 * pVectorFieldDensityFac;
    fieldY = 18 * pVectorFieldDensityFac;
    fieldPadding = 15;
    mainVecField = std::make_unique<VectorField>(fieldX, fieldY, fieldPadding, SDL_Color {0, 0, 0, 255});
    mainVecField->InitializeVectors(normalMapPixels, screenWidth, screenHeight, 20);

    // Create window and renderer
    window = (SDL_CreateWindow(WINDOW_NAME,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        screenWidth,
        screenHeight,
        0));
    renderer = SDL_CreateRenderer(window, -1, 0);

    CreateTextureFromPixelData(normalMapTexture, normalMapPixels[0], screenWidth, screenHeight, 3);

    for (auto& layer : layers)
        layer->UpdateLayerAll(false);

    mainVecField->UpdateAll();

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
            if (event.window.event == SDL_WINDOWEVENT_CLOSE)
            {
                int winID = event.window.windowID;
                if (winID == SDL_GetWindowID(window))
                {
                    isRunning = false;
                }
                else
                {
                    for (int i = 0; i < stitchResults.size(); i++)
                    {
                        StitchResult* res = stitchResults[i].get();
                        if (winID == SDL_GetWindowID(res->Get_Window()))
                        {
                            stitchResults.erase(stitchResults.begin() + i);
                            continue;
                        }
                    }
                }
            }
        }
        if (event.type == SDL_KEYDOWN)
        {
            SDL_Keycode pressedKey = event.key.keysym.sym;
            if (pressedKey == SDLK_q)
            {
                debugDisplay = !debugDisplay;
            }
            
            else if (pressedKey == SDLK_s)
            {
                std::string filename;
                std::string fileExtension;

                std::cout << "Save color map file as (supports .png, .bmp, .jpg): ";
                std::cin >> filename;

                SaveColorMap(filename, event.window.windowID);
            }

            else if (pressedKey == SDLK_DELETE)
            {
                DeleteSelectedPoints();
            }

            else if (pressedKey == SDLK_f)
            {
                if (sketchLines.empty()) continue;
                std::cout << sketchLines.size() << std::endl;
                sketchLines.back()->FlipPolarity();

                for (auto& pt : selectedPoints)
                {
                    pt.second->FlipPolarity();
                }
            }

            else if (pressedKey == SDLK_h)
            {
                CreateStitchDiagram();
            }

            else if (pressedKey == SDLK_d)
            {
                densityMode = !densityMode;
                const SDL_Color setCol = (densityMode) ? red : black;
                for (auto& pt : voronoiPoints)
                {
                    pt.second->Set_RenderColor(setCol);
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
        {
            for (auto& layer : layers)
                layer->CancelUpdate();
        }
        
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
                vPt->Set_RenderColor((densityMode) ? red : black);
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
            for (auto& layer : layers)
                layer->UpdateQueuedPixels();
        }
        else
        {
            for (auto& layer : layers)
                layer->CancelUpdate();
        }
    }

    leftMouseDownLastFrame = leftMouseHeld;
    middleMouseDownLastFrame = middleMouseHeld;
    rightMouseDownLastFrame = rightMouseHeld;

    void* pixelsToDisplay = (densityMode) ? densityMapPixels[0] : normalMapPixels[0];
    SDL_DestroyTexture(normalMapTexture);
    CreateTextureFromPixelData(normalMapTexture, pixelsToDisplay, screenWidth, screenHeight, 3);
}

void SketchProgram::Render()
{
    // Currently, background texture is same size as window which cannot be resized.
    SDL_RenderCopy(renderer, normalMapTexture, NULL, NULL);

    if (!sketchLines.empty() && leftMouseDownLastFrame)
    {
        sketchLines.back()->RenderLine(renderer, densityMode);
    }

    for (auto& layer : layers)
    {
        layer->RenderLayer(renderer, debugDisplay);
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
    sketchLines.clear();
    layers.clear();
    
    PixelRGB::DeleteContiguous2DPixmap(normalMapPixels);
    PixelRGB::DeleteContiguous2DPixmap(densityMapPixels);

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
}

void SketchProgram::SaveColorMap(std::string& filename, int winID)
{
    int errCheck;
    int format = SDL_PIXELFORMAT_RGB24;
    unsigned char* pixels = (unsigned char*) ((densityMode) ? densityMapPixels[0] : normalMapPixels[0]);
    SDL_Surface* pixelsToSurf;

    if (winID != SDL_GetWindowID(window))
    {
        StitchResult* foundResult = nullptr;
        for (auto& res : stitchResults)
        {
            if (SDL_GetWindowID(res->Get_Window()) == winID)
            {
                std::cout << "Saving from window " << SDL_GetWindowTitle(res->Get_Window()) << "\n";
                foundResult = res.get();
                break;
            }
        }

        if (foundResult == nullptr)
        {
            std::cout << "Invalid window id selected when saving.\n";
            return;
        }


        SDL_Renderer* rendLoc = foundResult->Get_Renderer();
        pixels = new unsigned char[screenWidth * screenHeight * 3];
        SDL_RenderReadPixels(rendLoc, NULL, format, pixels, 3 * screenWidth);
    }
    else
    {
        std::cout << "Saving from window " + std::string(WINDOW_NAME) + "\n";
    }

    pixelsToSurf = SDL_CreateRGBSurfaceWithFormatFrom(pixels, screenWidth, screenHeight, SDL_BITSPERPIXEL(format),
        screenWidth * SDL_BYTESPERPIXEL(format), format);

    filename = "output/images/" + filename;
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
        return;
    }
    else
        std::cout << "\nSaved image to " << filename << "\n";

    SDL_FreeSurface(pixelsToSurf);
    if (pixels != (unsigned char*)normalMapPixels[0] && pixels != (unsigned char*)densityMapPixels[0])
    {
        delete[] pixels;
    }
}

void SketchProgram::ReadParameters(const char* paramFile)
{
    // Read all paramters.
    std::ifstream pFile(paramFile);
    std::stringstream buffer;
    buffer << pFile.rdbuf();

    std::string fileStr = buffer.str();
    std::vector<std::string> args;
    std::string item;

    char delim = ' ';
    std::stringstream ss(fileStr);
    while (std::getline(ss, item, delim)) {
        args.push_back(item);
        delim = (delim == ' ') ? '\n' : ' ';
    }

    // Every odd
    pWidth = std::stoi(args[1]);
    pHeight = std::stoi(args[3]);
    pVectorFieldDensityFac = std::stod(args[5]);
    defaultNormalMap = args[7];
    defaultDensityMap = args[9];
    zoneMapName = args[11];

    // Print parameters so the user can verify they are what they wanted.
    std::cout << "Initializing with the following parameters: \n";
    std::cout << "Screen width: " << pWidth << "\n";
    std::cout << "Screen height: " << pHeight << "\n";
    std::cout << "Vector field density factor: " << pVectorFieldDensityFac << "\n";
    std::cout << "Static normal map: " << defaultNormalMap << "\n";
    std::cout << "Static density map: " << defaultDensityMap << "\n";
    std::cout << "Zone map: " << zoneMapName << "\n";
}

void SketchProgram::ParseZoneMap(const std::string& filename)
{
    int width, height, bytes;
    unsigned char* pixels;

    std::string path = "res/zonemaps/" + filename;
    pixels = stbi_load(path.c_str(), &width, &height, &bytes, 0);

    if (pixels == nullptr || filename.empty())
    {
        std::cout << "Invalid or no zone file given. Using one layer across screen.\n";
        width = std::min(1920, screenWidth);
        height = std::min(1920, screenHeight);
        bytes = 3;

        long totalLen = bytes * width * height;
        pixels = new unsigned char[totalLen];
        for (long i = 0; i < totalLen; i += 3)
        {
            pixels[i] = 128;
            pixels[i + 1] = 128;
            pixels[i + 2] = 255;
        }
    }
    else
    {
        // Scale pixels read to fit screen size.
        float xInc = (float)width / screenWidth;
        float yInc = (float)height / screenHeight;

        unsigned char* srcPixels = pixels;
        pixels = new unsigned char[screenWidth * screenHeight * 3];

        float approxX = 0.0f;
        for (int x = 0; x < screenWidth; ++x, approxX += xInc)
        {
            float approxY = 0.0f;
            for (int y = 0; y < screenHeight; ++y, approxY += yInc)
            {
                int indexNew = 3 * (y * screenWidth + x);
                int indexOld = bytes * ((int)approxY * width + (int)approxX);
                pixels[indexNew] = srcPixels[indexOld];
                pixels[indexNew + 1] = srcPixels[indexOld + 1];
                pixels[indexNew + 2] = srcPixels[indexOld + 2];
            }
        }

        stbi_image_free(srcPixels);
    }

    // NOW we read raw image data, and set zone information.
    std::vector<PixelRGB> uniqueColors;
    voronoiZonesByPixel.resize(screenWidth);
    PixelRGB whitePix = PixelRGB {255, 255, 255};
    for (int x = 0; x < screenWidth; x++)
    {
        voronoiZonesByPixel[x].resize(screenHeight);
        for (int y = 0; y < screenHeight; y++)
        {
            int index = bytes * (y * screenWidth + x);
            PixelRGB currPixel = PixelRGB();
            Uint8 alpha;

            currPixel.r = static_cast<unsigned char>(pixels[index]);
            currPixel.g = static_cast<unsigned char>(pixels[index + 1]);
            currPixel.b = static_cast<unsigned char>(pixels[index + 2]);
            alpha = (bytes >= 4) ? static_cast<unsigned char>(pixels[index + 3]) : 255;

            bool isNewColor = true;

            if (PixelRGB::Equals(&currPixel, &whitePix) || alpha == 0)
            {
                voronoiZonesByPixel[x][y] = 0;
                continue;
            }

            for (int i = 0; i < uniqueColors.size(); i++)
            {
                if (PixelRGB::Equals(&currPixel, &uniqueColors[i]))
                {
                    isNewColor = false;
                    voronoiZonesByPixel[x][y] = i + 1;
                    break;
                }
            }

            if (isNewColor)
            {
                uniqueColors.push_back(currPixel);
                voronoiZonesByPixel[x][y] = (int)uniqueColors.size();
            }
        }
    }

    // Initialize layers. Layer 0 will contain the contant normal/densitymap
    std::unique_ptr<Layer> bottom = std::make_unique<Layer>("res/normalmaps/" + defaultNormalMap, "res/densitymaps/" + defaultDensityMap, screenWidth, screenHeight, 0);
    layers.push_back(std::move(bottom));

    std::cout << (int)uniqueColors.size() << std::endl;

    for (int i = 0; i < uniqueColors.size(); ++i)
    {
        std::unique_ptr<Layer> newLayer = std::make_unique<Layer>(screenWidth, screenHeight, i + 1);
        layers.push_back(std::move(newLayer));
    }

    normalMapPixels = PixelRGB::CreateContiguous2DPixmap(screenHeight, screenWidth);
    densityMapPixels = PixelRGB::CreateContiguous2DPixmap(screenHeight, screenWidth);


    for (int x = 0; x < screenWidth; ++x)
        for (int y = 0; y < screenHeight; ++y)
        {
            int zone = voronoiZonesByPixel[x][y];
            layers[zone]->Set_PixelRefs(x, y, &normalMapPixels[y][x], &densityMapPixels[y][x]);
        }

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
            float interpX = (float)x / (sizeX - 1);
            float interpY = (float)y / (sizeY - 1);
            Vector2D pos = Vector2D((interpX * maxX) + padding, (interpY * maxY) + padding);

            int pixX = (int)pos[0];
            int pixY = (int)pos[1];
            int zone = voronoiZonesByPixel[pixX][pixY];

            if (layers[zone]->Get_IsEditable())
                std::cout << "Spawning Point: x = " << x << ", y = " << y << "\n";
            else
            {
                std::cout << "Unable to spawn point: x = " << x << ", y = " << y << ", position is uneditable!" << "\n";
                continue;
            }
            

            SDL_Color defCol;
            /*defCol.r = (Uint8)Helpers::Lerp(0, 255, interpX);
            defCol.g = (Uint8)Helpers::Lerp(0, 255, interpY);
            defCol.b = 128;
            defCol.a = 255;*/

            Helpers::NormalMapDefaultColor(&defCol);
            std::shared_ptr<VoronoiPoint> newPoint = std::make_shared<VoronoiPoint>(pos, defCol, zone);
            EmplaceVoronoiPoint(newPoint, false);
            newestPointID = newPoint->Get_ID();
            voronoiPoints[newestPointID] = std::move(newPoint);
        }

    for (auto& layer : layers)
    {
        layer->UpdateLayerAll(true);
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

void SketchProgram::EmplaceVoronoiPoint(std::shared_ptr<VoronoiPoint>& newPoint, bool updateAffectedBarycentric)
{
    layers[newPoint->Get_VoronoiZone()]->AddVoronoiPoint(newPoint, updateAffectedBarycentric);
}

SketchLine* SketchProgram::DrawSketchLine(bool placePoint)
{
    SketchLine* editLine = (sketchLines.empty()) ? nullptr : sketchLines.back().get();
    int zone = voronoiZonesByPixel[(int)mousePos[0]][(int)mousePos[1]];

    // If mouse was just clicked, create new line at mouse position.
    if (!leftMouseDownLastFrame)
    {
        editLine = new SketchLine(mousePos, mousePos);
        sketchLines.push_back(std::shared_ptr<SketchLine>(editLine));

        std::cout << zone << "\n";
        if (placePoint && layers[zone]->Get_IsEditable())
        {
            std::shared_ptr<VoronoiPoint> newPoint = std::make_shared<VoronoiPoint>(editLine->Get_Origin(), editLine->Get_RenderColor(), zone);
            newPoint->Set_RenderColor((densityMode) ? red : black);
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

    if (placePoint && layers[zone]->Get_IsEditable())
    {
        if (!densityMode)
        {
            voronoiPoints[newestPointID]->Set_NormalEncoding(editLine->Get_RenderColor());
            voronoiPoints[newestPointID]->Set_Polarity(editLine->Get_Polarity());
        }
        else
        {
            voronoiPoints[newestPointID]->Set_VoronoiDensity(editLine->GetPixelValueFromDistance(255, 0));
        }
    }
    mainVecField->UpdateAll();

    return editLine;
}

bool SketchProgram::PointRectOverlap(const SDL_Rect& aabb, const Vector2D& pt)
{
    return (aabb.x <= pt[0] && aabb.x + aabb.w >= pt[0] &&
        aabb.y <= pt[1] && aabb.y + aabb.h >= pt[1]);
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

void SketchProgram::RebuildMapNaive()
{
    for (auto& layer : layers)
    {
        layer->ClearData();
    }

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
    for (auto& layer : layers)
    {
        if (layer->Get_IsEditable())
            layer->UpdateLayerAll(!voronoiPoints.empty());
    }

    mainVecField->UpdateAll();
        
    std::cout << "Map Rebuilt!\n";
}

void SketchProgram::RecolorSelectedPoints(SketchLine* followLine)
{
    if (!leftMouseDownLastFrame)
    {
        for (auto& layer : layers)
            layer->RecolorSelectedPoints(selectedPoints);
    }

    if (!densityMode)
    {
        SDL_Color cacheCol = followLine->Get_RenderColor();
        bool cachePolarity = followLine->Get_Polarity();
        for (auto& vPt : selectedPoints)
        {
            vPt.second->Set_NormalEncoding(cacheCol);
            vPt.second->Set_Polarity(cachePolarity);
        }
    }
    else
    {
        // Set density of voronoi point based on followLine's length relative to 
        // the maximum length for controlling values (represented by white circle at runtime)
        // Density will range from 0 to 255
        int cacheVal = followLine->GetPixelValueFromDistance(255, 0);
        std::cout << cacheVal << "\n";
        for (auto& vPt : selectedPoints)
        {
            vPt.second->Set_VoronoiDensity(cacheVal);
        }
    }
}

void SketchProgram::DeleteSelectedPoints()
{
    for (auto& pt : selectedPoints)
    {
        // Should only be contained in one layer, but it doesn't take long to do this
        // anyway in case implemenation changes somehow.
        for (auto& layer : layers)
        {
            layer->RemovePoint(pt.second);
        }

        voronoiPoints.erase(pt.first);
    }

    selectedPoints.clear();

    RebuildMapNaive();
    selectedPoints.clear();
}

void SketchProgram::CreateStitchDiagram()
{ 
    /*std::cout << "Use premade density map from file (Y/N)?\nSelecting \"N\" will use currenly created density channel.";
    std::cin >> densName;
    densName.erase(std::remove_if(densName.begin(), densName.end(), isspace), densName.end());*/
    
    int width, height, bytes;
    unsigned char* pixels;
    PixelRGB** densityMap = nullptr;

    width = 100;
    height = 100;
    bytes = 3;

    std::unique_ptr<StitchResult> res = std::make_unique<StitchResult>(screenWidth, screenHeight, width, height, normalMapPixels, (densityMap == nullptr) ? densityMapPixels : densityMap);
    if (res->CreateStitches(true))
    {
        stitchResults.push_back(std::move(res));
    }
    
    if (densityMap != nullptr)
        PixelRGB::DeleteContiguous2DPixmap(densityMap);
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