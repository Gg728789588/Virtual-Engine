#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "../Common/RenderPackage.h"
#include "../RenderComponent/CBufferPool.h"
class Shader;
//Cube's drawing system
//Should be 
class CubeDrawer
{
private:
	
public:
	//Get a handle for each pipeline component
	uint GetHandleForPipelineComponent();
	//Add Cube To Rendering List
	void AddCube(uint3 pos, uint cubeID);
	//Remove Cube From Rendering List
	void RemoveCube(uint3 pos);
	//Clear Rendering Pipeline's state from last frame
	void ClearPipelineLastFrameState(
		uint handle);
	//Run ComputeShader Cull
	void Cull(
		uint handle,
		RenderPackage const& renderPackage,
		Math::Vector3* frustrumPlanes,
		Math::Vector3 frustumMinPos,
		Math::Vector3 frustumMaxPos);
	void Draw(
		uint handle,
		RenderPackage const& renderPackage,
		Shader const* drawShader,
		uint shaderPass
	);
	~CubeDrawer();
};