#include "io/CollisionIO.hpp"
#include <Archive.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "intersect.h"

// todo: thread this so we can show a loading screen
//std::atomic<bool> mCollisonGenerated;

void LCollisionIO::LoadMp(std::filesystem::path path, std::weak_ptr<LMapDOMNode> map){
    auto mapArc = map.lock()->GetArchive().lock();
    auto colFile = mapArc->GetFile("col.mp");

    bStream::CFileStream importCol(path.string(), bStream::OpenMode::In);

    unsigned char* importColData = new unsigned char[importCol.getSize()];
    importCol.readBytesTo(importColData, importCol.getSize());

    colFile->SetData(importColData, importCol.getSize());

    delete[] importColData;
}

void LCollisionIO::LoadObj(std::filesystem::path path, std::weak_ptr<LMapDOMNode> map){    
    
    glm::vec3 bbmin, bbmax;

    int tangentIdx = 0;
    std::vector<glm::vec3> normals { glm::vec3(0.0f, 0.0f, 0.0f) };
    std::vector<CollisionTriangle> triangles;
    std::vector<GridCell> grid; 

    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;
    bool ret = tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, path.string().c_str(), nullptr, true);

    if(!ret){
        return;
    }

    tangentIdx = attributes.normals.size();

    for(auto shp : shapes){
        for(int poly = 0; poly < shp.mesh.indices.size() / 3; poly++){
            CollisionTriangle tri;
            
            std::cout << "[CollisionIO:ObjImport]: Shape Point Count - " << shp.mesh.indices.size() << std::endl;
            if(shp.mesh.indices.size() == 0) continue;

            tri.mVtx1 = shp.mesh.indices[(3*poly)+0].vertex_index;
            tri.mVtx2 = shp.mesh.indices[(3*poly)+1].vertex_index;
            tri.mVtx3 = shp.mesh.indices[(3*poly)+2].vertex_index;

            glm::vec3 v1 = glm::vec3(attributes.vertices[tri.mVtx1 * 3], attributes.vertices[(tri.mVtx1 * 3) + 1], attributes.vertices[(tri.mVtx1 * 3) + 2]);
            glm::vec3 v2 = glm::vec3(attributes.vertices[tri.mVtx2 * 3], attributes.vertices[(tri.mVtx2 * 3) + 1], attributes.vertices[(tri.mVtx2 * 3) + 2]);
            glm::vec3 v3 = glm::vec3(attributes.vertices[tri.mVtx3 * 3], attributes.vertices[(tri.mVtx3 * 3) + 1], attributes.vertices[(tri.mVtx3 * 3) + 2]);

            tri.v1 = v1;
            tri.v2 = v2;
            tri.v3 = v3;

            bbmin = glm::vec3(glm::min(bbmin.x, v1.x), glm::min(bbmin.y, v1.y), glm::min(bbmin.z, v1.z));
            bbmin = glm::vec3(glm::min(bbmin.x, v2.x), glm::min(bbmin.y, v2.y), glm::min(bbmin.z, v2.z));
            bbmin = glm::vec3(glm::min(bbmin.x, v3.x), glm::min(bbmin.y, v3.y), glm::min(bbmin.z, v3.z));

            bbmax = glm::vec3(glm::min(bbmax.x, v1.x), glm::min(bbmax.y, v1.y), glm::min(bbmax.z, v1.z));
            bbmax = glm::vec3(glm::min(bbmax.x, v2.x), glm::min(bbmax.y, v2.y), glm::min(bbmax.z, v2.z));
            bbmax = glm::vec3(glm::min(bbmax.x, v3.x), glm::min(bbmax.y, v3.y), glm::min(bbmax.z, v3.z));

            glm::vec3 e10 = v2 - v1; // u
            glm::vec3 e20 = v3 - v1; // v

            glm::vec3 e01 = v1 - v2;
            glm::vec3 e21 = v3 - v2; 


            glm::vec3 faceNormal = {
                (e10.y * e20.z) - (e10.z * e20.y),
                (e10.z * e20.x) - (e10.x * e20.z),
                (e10.x * e20.y) - (e10.y * e20.x)
            };

            faceNormal = glm::normalize(faceNormal);
            
            if(std::find(normals.begin(), normals.end(), faceNormal) == normals.end()) normals.push_back(faceNormal);
            tri.mNormal = std::distance(std::find(normals.begin(), normals.end(), faceNormal), normals.begin());

            // Tangents
            glm::vec3 tan1 = glm::normalize(glm::cross(faceNormal, e10));
            glm::vec3 tan2 = glm::normalize(glm::cross(faceNormal, e20));
            glm::vec3 tan3 = glm::normalize(glm::cross(faceNormal, e21));
            
            if(std::find(normals.begin(), normals.end(), tan1) == normals.end()) normals.push_back(tan1);
            tri.mEdgeTan1 = std::distance(std::find(normals.begin(), normals.end(), tan1), normals.begin());

            if(std::find(normals.begin(), normals.end(), tan2) == normals.end()) normals.push_back(tan2);
            tri.mEdgeTan2 = std::distance(std::find(normals.begin(), normals.end(), tan2), normals.begin());

            if(std::find(normals.begin(), normals.end(), tan3) == normals.end()) normals.push_back(tan3);
            tri.mEdgeTan3 =std::distance(std::find(normals.begin(), normals.end(), tan3), normals.begin());
            
            glm::vec3 up(0.0f, 1.0f, 0.0f);
            float numerator = glm::dot(faceNormal, up);
            float denomenator = faceNormal.length() * up.length();
            float angle = (float)glm::acos(numerator / denomenator);

            angle *= (float)(180 / glm::pi<float>());
            if(glm::abs(angle) >= 0.0f && glm::abs(angle) <= 0.5f){
                tri.mUnkIdx = tri.mEdgeTan2;
            } else {
                tri.mUnkIdx = 0;
            }

            if(glm::abs(angle) <= 80.0f){
                tri.mFloor = true;
            }
            
            tri.mDot = glm::dot(tan3, e10);

            tri.mMask = 0x8000;
            tri.mFriction = 0;

            triangles.push_back(tri);
        }
    }

    glm::vec3 axisLengths((bbmax.x - bbmin.x), (bbmax.y - bbmin.y), (bbmax.z - bbmin.z));

    std::vector<uint16_t> groupData = { 0xFFFF };

    // Now that we have all the triangles, generate the grid and the groups.
	int xCellCount = (int)(glm::floor(axisLengths.x / 256.0f) + 1);
	int yCellCount = (int)(glm::floor(axisLengths.y / 512.0f) + 1);
	int zCellCount = (int)(glm::floor(axisLengths.z / 256.0f) + 1);

    float xCellSize = axisLengths.x / xCellCount;
    float yCellSize = axisLengths.y / yCellCount;
    float zCellSize = axisLengths.z / zCellCount;

    float xHalfSize = xCellSize / 2;
    float yHalfSize = yCellSize / 2;
    float zHalfSize = zCellSize / 2;

    float curX = bbmin.x + xHalfSize;
    float curY = bbmin.y + yHalfSize;
    float curZ = bbmin.z + zHalfSize;

    for (int z = 0; z < zCellCount; z++){
        for (int y = 0; y < yCellCount; y++){
            for (int x = 0; x < xCellCount; x++){
                for(std::vector<CollisionTriangle>::iterator tri = triangles.begin(); tri != triangles.end(); tri++){
                    GridCell cell;

                    float boxCenter[3] = {curX, curY, curZ};
                    float halfSize[3] = {xHalfSize, yHalfSize, zHalfSize};
                    float verts[3][3] = {
                        {tri->v1.x, tri->v1.y, tri->v1.z},
                        {tri->v2.x, tri->v2.y, tri->v2.z},
                        {tri->v3.x, tri->v3.y, tri->v3.z}
                    };

                    std::vector<uint16_t> floorTris = {};
                    cell.mAllGroupIdx = groupData.size();
                    bool empty = true;
                    if(triBoxOverlap(boxCenter, halfSize, verts) != 0){
                        // add to cell all group!
                        groupData.push_back(tri - triangles.begin());    
                        // add to cell floor group!
                        if(tri->mFloor) floorTris.push_back(tri - triangles.begin());
                        empty = false;
                    }

                    if(!empty){
                        cell.mFloorGroupIdx = groupData.size();
                        for(auto tri : floorTris){
                            groupData.push_back(tri);
                        }
                        groupData.push_back(0xFFFF);
                    } else {
                        cell.mAllGroupIdx = 0;
                        cell.mFloorGroupIdx = 0;
                    }


                    grid.push_back(cell);
                }
            }
            curX = bbmin.x + xHalfSize;
            curY += yCellSize;
        }
        curY = bbmin.y + yHalfSize;
        curZ += zCellSize;
    }
    groupData.push_back(0xFFFF);


    // Write structures to file
    
    bStream::CMemoryStream stream(1024, bStream::Endianess::Big, bStream::OpenMode::Out);

    // Write scale
    stream.writeFloat(256.0f);
    stream.writeFloat(512.0f);
    stream.writeFloat(256.0f);

    // Write Min + Axis Len
    stream.writeFloat(bbmin.x);
    stream.writeFloat(bbmin.y);
    stream.writeFloat(bbmin.z);

    stream.writeFloat(axisLengths.x);
    stream.writeFloat(axisLengths.y);
    stream.writeFloat(axisLengths.z);

    stream.writeUInt32(0x40);
    stream.writeUInt32(0);
    stream.writeUInt32(0);
    stream.writeUInt32(0);
    stream.writeUInt32(0);
    stream.writeUInt32(0);
    stream.writeUInt32(0);

    for(int v = 0; v < attributes.vertices.size(); v++){
        stream.writeFloat(attributes.vertices[v]);
    }

    uint32_t normalsOffset = stream.tell();

    for(std::vector<glm::vec3>::iterator n = normals.begin(); n != normals.end(); n++){
        stream.writeFloat(n->x);
        stream.writeFloat(n->y);
        stream.writeFloat(n->z);
    }

    uint32_t trianglesOffset = stream.tell();

    for(std::vector<CollisionTriangle>::iterator t = triangles.begin(); t != triangles.end(); t++){
        stream.writeUInt16(t->mVtx1);
        stream.writeUInt16(t->mVtx2);
        stream.writeUInt16(t->mVtx3);

        stream.writeUInt16(t->mNormal);

        stream.writeUInt16(t->mEdgeTan1);
        stream.writeUInt16(t->mEdgeTan2);
        stream.writeUInt16(t->mEdgeTan3);

        stream.writeUInt16(t->mUnkIdx);

        stream.writeFloat(t->mDot);

        stream.writeUInt16(t->mMask);
        stream.writeUInt16(t->mFriction);
    }

    uint32_t groupOffset = stream.tell();

    for(std::vector<uint16_t>::iterator t = groupData.begin(); t != groupData.end(); t++){
        stream.writeUInt16(*t);
    }

    uint32_t gridOffset = stream.tell();

    for(std::vector<GridCell>::iterator t = grid.begin(); t != grid.end(); t++){
        stream.writeUInt32(t->mAllGroupIdx);
        stream.writeUInt32(t->mFloorGroupIdx);
    }

    uint32_t unkData = stream.tell();

    long aligned = (stream.tell() + 31) & ~31;
    long delta = aligned - stream.tell();

    for(int x = 0; x < delta; x++){
        stream.writeUInt8(0x40);
    }

    stream.seek(0x28);
    stream.writeUInt32(normalsOffset);
    stream.writeUInt32(trianglesOffset);
    stream.writeUInt32(groupOffset);
    stream.writeUInt32(gridOffset);
    stream.writeUInt32(gridOffset);
    stream.writeUInt32(unkData);
    stream.seek(0);

    auto mapArc = map.lock()->GetArchive().lock();
    auto colFile = mapArc->GetFile("col.mp");

    colFile->SetData(stream.getBuffer(), stream.getSize());

}