#include "DOM/RoomDataDOMNode.hpp"
#include "DOM/RoomDOMNode.hpp"
#include "io/StaticMapDataIO.hpp"

LRoomDataDOMNode::LRoomDataDOMNode(std::string name) : Super(name)
{
	mType = EDOMNodeType::RoomData;
}

void LRoomDataDOMNode::RenderDetailsUI(float dt)
{

}

bool LRoomDataDOMNode::Load(const uint32_t& index, const LStaticMapDataIO& source, const std::vector<std::shared_ptr<LRoomDOMNode>>& mapRooms, const std::vector<std::shared_ptr<LDoorDOMNode>>& mapDoors)
{
	// Room data
	LStaticRoomData roomData;
	source.GetRoomData(index, roomData);

	mCameraPositionIndex = roomData.mCameraPositionIndex;
	mFloor = roomData.mFloor;
	mDoorZone = roomData.mDoorZone;
	mRoomID = roomData.mRoomID;

	mCameraBehavior = roomData.mCameraBehavior;

	DeconstructBoundingBox(roomData.mBoundingBoxMin, roomData.mBoundingBoxMax);

	mUnknown1 = roomData.mUnknown1;
	mUnknown2 = roomData.mUnknown2;

	mDarkColor = roomData.mDarkColor;

	// Resource path
	source.GetRoomResourcePath(index, mResourcePath);

	// Doors
	source.GetDoorListData(roomData.mDoorListIndex, mDoorListIndices);

	for (uint16_t dIndex : mDoorListIndices)
		mDoorList.push_back(mapDoors[dIndex]);

	// Adjacent rooms
	source.GetAdjacentRoomListData(index, mAdjacentRoomIndices);

	for (uint16_t rIndex : mAdjacentRoomIndices)
		mAdjacentRooms.push_back(mapRooms[rIndex]);

	return true;
}

bool LRoomDataDOMNode::Save(LStaticMapDataIO& dest)
{
	return true;
}

void LRoomDataDOMNode::DeconstructBoundingBox(const glm::vec3& min, const glm::vec3& max)
{
	mScale = max - min;

	mPosition = min + (mScale / 2.f);
}

void LRoomDataDOMNode::ConstructBoundingBox(glm::vec3& min, glm::vec3& max)
{
	glm::vec3 halfExtents = mScale / 2.f;

	min = mPosition - halfExtents;
	max = mPosition + halfExtents;
}