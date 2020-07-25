#pragma once
//#include "../RenderComponent/MeshRenderer.h"
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "../Common/BitArray.h"
#include "../Common/RandomVector.h"
#include "../Common/ArrayList.h"
#include <mutex>
class FrameResource;
class GameTimer;
class Transform;
class DescriptorHeap;
class Mesh;
class MeshRenderer;
class JobBucket;
class Texture;
class Camera;
class Light;
class JobHandle;
class Skybox;
class CubeDrawer;

//Only For Test!
class World final
{
	friend class Transform;
private:
	World(ID3D12GraphicsCommandList* cmdList, ID3D12Device* device);
	static World* current;
	std::mutex mtx;
	RandomVector<ObjectPtr<Transform>> allTransformsPtr;
	ObjectPtr<CubeDrawer> cubeDrawer;
	vengine::vector<ObjectPtr<Camera>> allCameras;
public:

	//ObjectPtr<MeshRenderer> testMeshRenderer;
	std::wstring windowInfo;
	ObjectPtr<Light> testLight;
	ObjectPtr<Skybox> currentSkybox;
	vengine::vector<ObjectPtr<Camera>>& GetCameras()
	{
		return allCameras;
	}
	CubeDrawer* GetCubeDrawer() const noexcept {
		return cubeDrawer;
	}
	~World();
	void DestroyAllCameras();
	UINT windowWidth;
	UINT windowHeight;
	inline static constexpr World* GetInstance() { return current; }
	inline static constexpr World* CreateInstance(ID3D12GraphicsCommandList* cmdList, ID3D12Device* device)
	{
		if (current)
			return current;
		new World(cmdList, device);
		return current;
	}
	inline static constexpr void DestroyInstance()
	{
		auto a = current;
		current = nullptr;
		if (a) delete a;
	}
	void Update(FrameResource* resource, ID3D12Device* device, GameTimer& timer, uint64 frameIndex, int2 screenSize, const Math::Vector3& moveDir);
};