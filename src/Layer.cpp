#include "Layer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

Layer::Layer(int sizeX, int sizeY, int zone)
{
    this->sizeX = sizeX;
    this->sizeY = sizeY;

    rawNormalData = PixelRGB::CreateContiguous2DPixmap(sizeX, sizeY);
    rawDensityData = PixelRGB::CreateContiguous2DPixmap(sizeX, sizeY);

    normalMap.resize(sizeX);
    for (int x = 0; x < sizeX; x++)
    {
        std::vector<DynamicColor*> temp;
        temp.reserve(sizeY);
        for (int y = 0; y < sizeY; y++)
        {
            // Initialize with pixel from allocated pixmap. However visible pixels will be overrwritten (with Set_PixelRefs)
            DynamicColor* newCol = new DynamicColor(&rawNormalData[y][x], &rawDensityData[y][x], Vector2D(x, y));
            newCol->SetVoronoiZone(zone);
            temp.push_back(newCol);
        }
        normalMap[x].insert(normalMap[x].end(), temp.begin(), temp.end());
    }

    pixelsToUpdate.reserve(normalMap.size() * normalMap[0].size());
}

Layer::Layer(const std::string& normalName, const std::string& densityName, int sizeX, int sizeY, int zone)
{
    this->editable = false;
    this->sizeX = sizeX;
    this->sizeY = sizeY;

    int width, height, bytes;
    unsigned char* pixels = stbi_load(normalName.c_str(), &width, &height, &bytes, 0);
    rawNormalData = PixelRGB::CreateContiguous2DPixmap(sizeY, sizeX, true);

    if (pixels != nullptr && normalName != "")
    {
        float xInc = (float)width / sizeX;
        float yInc = (float)height / sizeY;

        float approxX = 0.0f;
        for (int x = 0; x < sizeX; ++x, approxX += xInc)
        {
            float approxY = 0.0f;
            for (int y = 0; y < sizeY; ++y, approxY += yInc)
            {
                int indexOld = bytes * ((int)approxY * width + (int)approxX);
                rawNormalData[y][x].r = pixels[indexOld];
                rawNormalData[y][x].g = pixels[indexOld + 1];
                rawNormalData[y][x].b = pixels[indexOld + 2];
            }
        }
    }

    stbi_image_free(pixels);

    // Do same for density map.
    pixels = stbi_load(densityName.c_str(), &width, &height, &bytes, 0);
    rawDensityData = PixelRGB::CreateContiguous2DPixmap(sizeY, sizeX);

    if (pixels != nullptr && densityName != "")
    {
        float xInc = (float)width / sizeX;
        float yInc = (float)height / sizeY;

        float approxX = 0.0f;
        for (int x = 0; x < sizeX; ++x, approxX += xInc)
        {
            float approxY = 0.0f;
            for (int y = 0; y < sizeY; ++y, approxY += yInc)
            {
                int indexOld = bytes * ((int)approxY * width + (int)approxX);
                rawDensityData[y][x].r = pixels[indexOld];
                rawDensityData[y][x].g = pixels[indexOld + 1];
                rawDensityData[y][x].b = pixels[indexOld + 2];
            }
        }
    }
    else
    {
        for (int x = 0; x < sizeX; ++x)
            for (int y = 0; y < sizeY; ++y)
            {
                int index = bytes * (y * sizeX + x);
                rawDensityData[y][x].r = 0;
                rawDensityData[y][x].g = 0;
                rawDensityData[y][x].b = 0;
            }

    }

    stbi_image_free(pixels);

    normalMap.resize(sizeX);
    for (int x = 0; x < sizeX; x++)
    {
        std::vector<DynamicColor*> temp;
        temp.reserve(sizeY);
        for (int y = 0; y < sizeY; y++)
        {
            // Initialize with pixel from allocated pixmap. However visible pixels will be overrwritten (with Set_PixelRefs)
            DynamicColor* newCol = new DynamicColor(&rawNormalData[y][x], &rawDensityData[y][x], Vector2D(x, y));
            newCol->SetVoronoiZone(zone);
            temp.push_back(newCol);
        }
        normalMap[x].insert(normalMap[x].end(), temp.begin(), temp.end());
    }

    for (int x = 0; x < sizeX; x++)
    {
        std::vector<DynamicColor*> temp;
        temp.reserve(sizeY);
        for (int y = 0; y < sizeY; y++)
        {
            // Initialize with pixel from allocated pixmap. However visible pixels will be overrwritten (with Set_PixelRefs)
            DynamicColor* newCol = new DynamicColor(&rawNormalData[y][x], &rawDensityData[y][x], Vector2D(x, y));
            newCol->SetVoronoiZone(zone);
            temp.push_back(newCol);
        }
        normalMap[x].insert(normalMap[x].end(), temp.begin(), temp.end());
    }

    UpdateLayerAll(true);

    pixelsToUpdate.reserve(normalMap.size() * normalMap[0].size());
}

Layer::~Layer()
{
    PixelRGB::DeleteContiguous2DPixmap(rawNormalData);
    PixelRGB::DeleteContiguous2DPixmap(rawDensityData);
}

void Layer::AddVoronoiPoint(const std::shared_ptr<VoronoiPoint>& newPoint, bool updateBarycentric)
{
    ownedPoints.emplace(newPoint->Get_ID(), newPoint);

    int floorPosX = newPoint->Get_Position()[0];
    int floorPosY = newPoint->Get_Position()[1];

    std::vector<DynamicColor*> pixelsToEvaluate;
    std::vector<int> redundantNodes;

    // Pixels must be updated if their min point is overrwritten.
    for (int x = 0; x < sizeX; x++)
        for (int y = 0; y < sizeY; y++)
        {
            DynamicColor* it = normalMap[x][y];
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
        int zone = pix->Get_VoronoiZone();

        //std::cout << pix->Get_PixelPosition()[0] << " " << pix->Get_PixelPosition()[1] << "\n";

        int OOBx = 0;   // Determines if we HAVE GONE out of bounds at some point thus far (sign says direction)
        int OOBy = 0;   // Same as above, for y-direction.
        std::vector<VoronoiPoint*> uniquePoints;
        Vector2D averagePos = Vector2D(pixX, pixY);
        uniquePoints.push_back(newPoint.get());
        for (int x = -1; x <= 1; x++)
        {
            int checkX = pixX + x;
            OOBx = (OOBx == 0) ? ((checkX < 0) ? -1 : (checkX >= sizeX) ? 1 : 0) : OOBx;

            for (int y = -1; y <= 1; y++)
            {
                int checkY = pixY + y;
                OOBy = (OOBy == 0) ? ((checkY < 0) ? -1 : (checkY >= sizeY) ? 1 : 0) : OOBy;

                if (checkY < 0 || checkY >= sizeY) continue;
                if (checkX < 0 || checkX >= sizeX) continue;

                bool contains = false;
                VoronoiPoint* add = normalMap[checkX][checkY]->Get_MinPoint();
                if (add == nullptr) break;

                affectedPoints[add->Get_ID()] = add;
                for (auto* pt : uniquePoints)
                {
                    if (pt->Get_ID() == normalMap[checkX][checkY]->Get_MinPoint()->Get_ID())
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

    // Yikes ( O(n^3) )...
    // But we want to update any pixels that have been affected by the change, which
    // will involve pixels outside of the new voronoi zone.
    for (auto& pt : affectedPoints)
    {
        if (updateBarycentric)
        {
            for (int x = 0; x < sizeX; x++)
            {
                for (int y = 0; y < sizeY; y++)
                {
                    VoronoiPoint* minPt = normalMap[x][y]->Get_MinPoint();
                    if (minPt == nullptr) continue;

                    if (minPt->Get_ID() == pt.first && minPt->Get_VoronoiZone() == pt.second->Get_VoronoiZone())
                    {
                        pixelsToUpdate.push_back(normalMap[x][y]);
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
    if (updateBarycentric)
        BarycentricUpdate(pixelsToUpdate);
}

void Layer::RemovePoint(const std::shared_ptr<VoronoiPoint>& toRemove)
{
    ownedPoints.erase(toRemove->Get_ID());
    for (auto& node : toRemove->Get_NeighboringNodes())
    {
        createdNodes.erase(node->Get_ID());
    }
}

void Layer::RecolorSelectedPoints(std::unordered_map<int, std::shared_ptr<VoronoiPoint>>& selectedPoints)
{
    pixelsToUpdate.clear();

    // If there's any room f
    for (int x = 0; x < sizeX; x++)
        for (int y = 0; y < sizeY; y++)
        {
            DynamicColor* it = normalMap[x][y];
            if (it->Get_TriNodeA() == nullptr || it->Get_TriNodeB() == nullptr)
                continue;

            // Pixel should update if it's barcentric blending is involved with the points'
            // nodes, since those nodes are influenced by the points themselves.
            for (auto& ptA : it->Get_TriNodeA()->Get_IntersectingPoints())
            {
                if (selectedPoints.count(ptA->Get_ID()) >= 1)
                {
                    pixelsToUpdate.push_back(it);
                    continue;
                }
            }

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

void Layer::UpdateLayerAll(bool barycentric)
{
    if (ownedPoints.size() == 0) return;

    for (int x = 0; x < sizeX; x++)
    {
        if (barycentric) BarycentricUpdate(normalMap[x]);
        for (int y = 0; y < sizeY; y++)
        {
            normalMap[x][y]->UpdatePixel();
        }
    }
}

void Layer::CancelUpdate()
{
    pixelsToUpdate.clear();
}

void Layer::ClearData()
{
    for (int x = 0; x < sizeX; x++)
        for (int y = 0; y < sizeY; y++)
        {
            normalMap[x][y]->ClearVoronoiData(editable);
        }

    for (auto& vPt : ownedPoints)
    {
        vPt.second->ClearNodes();
    }

    createdNodes.clear();
    ownedPoints.clear();
}

void Layer::UpdateQueuedPixels()
{
    for (auto& pix : pixelsToUpdate)
    {
        pix->UpdatePixel();
    }
}

void Layer::RenderLayer(SDL_Renderer* rend, bool debugDisplay)
{
    for (auto& pt : ownedPoints)
    {
        pt.second->RenderPoint(rend);
    }

    if (debugDisplay)
    {
        for (auto& pt : ownedPoints)
        {
            pt.second->RenderFormedTriangles(rend);
        }

        for (auto& node : createdNodes)
        {
            node.second->RenderNode(rend);
        }
    }
}

void Layer::Set_PixelRefs(int x, int y, PixelRGB* normalPix, PixelRGB* densityPix)
{
    PixelRGB::Copy(&rawNormalData[y][x], normalPix);
    PixelRGB::Copy(&rawDensityData[y][x], densityPix);
	return normalMap[x][y]->Set_AffectedPixels(normalPix, densityPix);
}

void Layer::CheckForIntersections(const std::vector<DynamicColor*>& toCheck)
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
            OOBx = (OOBx == 0) ? ((checkX < 0) ? -1 : (checkX >= sizeX) ? 1 : 0) : OOBx;

            for (int y = -1; y <= 1; y++)
            {
                int checkY = pixY + y;
                OOBy = (OOBy == 0) ? ((checkY < 0) ? -1 : (checkY >= sizeY) ? 1 : 0) : OOBy;

                if (checkY < 0 || checkY >= sizeY) continue;
                if (checkX < 0 || checkX >= sizeX) continue;

                bool contains = false;
                VoronoiPoint* add = normalMap[checkX][checkY]->Get_MinPoint();
                if (add == nullptr) break;

                for (auto* pt : uniquePoints)
                {
                    if (pt->Get_ID() == normalMap[checkX][checkY]->Get_MinPoint()->Get_ID())
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

void Layer::BarycentricUpdate(const std::vector<DynamicColor*>& toUpdate)
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