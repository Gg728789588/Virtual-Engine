#pragma once
#include "../RenderComponent/MeshRenderer.h"
#include "../Singleton/MathLib.h"

class CullTask
{
public:
	struct CullingResult
	{
		UINT rendererIndex;
		UINT submeshIndex;
	};
private:
	vengine::vector<MeshRenderer*>* waitingBoundingBox;
	vengine::vector<CullingResult> cullingResult;
	DirectX::XMVECTOR cameraCullingPlanes[6];
	//JobHandler waitingHandler;
	//tf::Task waitingTask;
public:
	vengine::vector<CullingResult>* GetCullingResult();
	//tf::Task& GetTask() { return waitingTask; }
	/*void ScheduleCullingJob(
		vengine::vector<MeshRenderer*>* targets,
		DirectX::XMVECTOR* cullingPlanes,
		tf::Taskflow& flow);*/
};
