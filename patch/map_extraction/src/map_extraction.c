#include "map_extraction.h"
#include "util.h"

#include <stddef.h>

LMapFile* FileBuffer = NULL;

uint32_t TryLoadMapFile(uint16_t MapID)
{
  // Generate the path to the map file.
  char FormattedPath[32];
  snprintf(FormattedPath, 32, "/Iwamoto/map%d/rooms.map", MapID);
  
  // Query the DVD to check if the map file exists; if not, early out.
  uint32_t DoesFileExist = DVDConvertPathToEntrynum(FormattedPath);
  if (DoesFileExist == -1)
    return 0;
  
  // Grab the info about the map file from the DVD.
  DVDFileInfo FileInfo;
  DVDOpen(FormattedPath, &FileInfo);
  
  // Allocate memory to hold the map file.
  uint32_t FileSize = OSRoundUp32B(FileInfo.mLength);
  FileBuffer = (LMapFile*)JKRHeap_alloc(FileSize, 32, NULL);
  
  // Read the map file from the DVD.
  DVDReadPrio(&FileInfo, (char*)FileBuffer, FileSize, NULL, 2);
  DVDClose(&FileInfo);
  
  return 1;
}

void UpdateMapFileOffsets()
{
  // Header offsets
  FileBuffer->mRoomData = offset_to_ptr(FileBuffer, (uint32_t)FileBuffer->mRoomData);
  FileBuffer->mRoomResourcePaths = offset_to_ptr(FileBuffer, (uint32_t)FileBuffer->mRoomResourcePaths);
  FileBuffer->mAltResourceData = offset_to_ptr(FileBuffer, (uint32_t)FileBuffer->mAltResourceData);
  FileBuffer->mDoorData = offset_to_ptr(FileBuffer, (uint32_t)FileBuffer->mDoorData);
  FileBuffer->mDoorListData = offset_to_ptr(FileBuffer, (uint32_t)FileBuffer->mDoorListData);
  FileBuffer->mRoomAdjacencyList = offset_to_ptr(FileBuffer, (uint32_t)FileBuffer->mRoomAdjacencyList);
  
  // Room doors
  for (uint32_t i = 0; i < FileBuffer->mRoomCount; i++)
    FileBuffer->mRoomData[i].mDoorListRef = offset_to_ptr(FileBuffer->mDoorListData, (uint32_t)FileBuffer->mRoomData[i].mDoorListRef);
  
  // Room resources
  for (uint32_t i = 0; i < FileBuffer->mRoomCount; i++)
    FileBuffer->mRoomResourcePaths[i] = offset_to_ptr(FileBuffer->mRoomResourcePaths, (uint32_t)FileBuffer->mRoomResourcePaths[i]);
  
  // Alt room resources
  for (uint32_t i = 0; i < FileBuffer->mAltResourceCount; i++)
    FileBuffer->mAltResourceData[i].mPath = offset_to_ptr(FileBuffer->mAltResourceData, (uint32_t)FileBuffer->mAltResourceData[i].mPath);
  
  // Room adjacency list
  for (uint32_t i = 0; i < FileBuffer->mRoomAdjacencyListCount; i++)
    FileBuffer->mRoomAdjacencyList[i] = offset_to_ptr(FileBuffer->mRoomAdjacencyList, (uint32_t)FileBuffer->mRoomAdjacencyList[i]);
}

void InitMap_External(uint16_t MapID)
{
  FileBuffer = NULL;
  
  if (TryLoadMapFile(MapID))
  {
    UpdateMapFileOffsets();
    
    LMapData* Map = MapDataPtrs[MapID];
    
    Map->mRoomCount = FileBuffer->mRoomCount;
    Map->mRoomCount2 = FileBuffer->mRoomCount;
    
    Map->mRoomDefs = FileBuffer->mRoomData;
    Map->mRoomResPaths = FileBuffer->mRoomResourcePaths;
    Map->mAltRoomModelDefs = FileBuffer->mAltResourceData;
        
    Map->mRoomAdjacencyLists = FileBuffer->mRoomAdjacencyList;
    Map->mDoorDefs = FileBuffer->mDoorData;
  }
    
  // Continue with native map initializing.
  InitMap(MapID);
}

void FreeMap_External()
{
  // Free the memory where the map file was stored, if it was loaded in the first place.
  if (FileBuffer != NULL)
  {
     JKRHeap_free(FileBuffer, NULL);
     FileBuffer = NULL;     
  }
  
  // Continue with native map destruction.
  FreeMap();
}