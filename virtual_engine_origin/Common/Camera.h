//***************************************************************************************
// Camera.h by Frank Luna (C) 2011 All Rights Reserved.
//   
// Simple first person style camera class that lets the viewer explore the 3D scene.
//   -It keeps track of the camera coordinate system relative to the world space
//    so that the view matrix can be constructed.  
//   -It keeps track of the viewing frustum of the camera so that the projection
//    matrix can be obtained.
//***************************************************************************************

#ifndef CAMERA_H
#define CAMERA_H

#include "d3dUtil.h"
#include "../Common/MObject.h"
#include "../PipelineComponent/IPipelineResource.h"
#include "../Common/Runnable.h"
class PipelineComponent;
class RenderPipeline;
class RenderTexture;
class PassConstants;
class FrameResource;
class Camera : public MObject
{
	friend class PipelineComponent;
	friend class RenderPipeline;
public:
	enum CameraRenderPath
	{
		DefaultPipeline = 0
	};

	RenderTexture* renderTarget = nullptr;
	bool autoRender = true;
	virtual ~Camera();
	Camera(ID3D12Device* device, CameraRenderPath rtType);
	// Get/Set world camera position.
	Math::Vector3 const& GetPosition() const { return mPosition; }
	void SetPosition(double x, double y, double z);
	void SetPosition(const DirectX::XMFLOAT3& v);

	// Get camera basis vectors.
	Math::Vector3 const& GetRight()const { return mRight; }
	Math::Vector3 const& GetUp()const { return mUp; }
	Math::Vector3 const& GetLook()const { return mLook; }

	// Get frustum properties.
	double GetNearZ()const { return mNearZ; }
	double GetFarZ()const { return mFarZ; }
	double GetAspect()const { return mAspect; }
	double GetFovY()const { return mFovY; }
	double GetFovX()const {
		double halfWidth = 0.5 * GetNearWindowWidth();
		return 2.0 * atan(halfWidth / mNearZ);
	}

	// Get near and far plane dimensions in view space coordinates.
	double GetNearWindowWidth()const { return mAspect * mNearWindowHeight; }
	double GetNearWindowHeight()const { return mNearWindowHeight; }
	double GetFarWindowWidth()const { return mAspect * mFarWindowHeight; }
	double GetFarWindowHeight()const { return mFarWindowHeight; }

	// Set frustum.
	void SetLens(double fovY, double aspect, double zn, double zf);

	// Define camera space via LookAt parameters.
	void LookAt(const Math::Vector3& pos, const Math::Vector3& target, const Math::Vector3& worldUp);
	// Get View/Proj matrices.
	Math::Matrix4 const& GetView()const { return mView; }
	Math::Matrix4 const& GetCameraToWorld() const { return cameraToWorld; }
	Math::Matrix4 const& GetProj()const { return mProj; }
	void SetProj(const DirectX::XMFLOAT4X4& data);
	void SetProj(const Math::Matrix4& data);
	void SetView(const DirectX::XMFLOAT4X4& data);
	void SetView(const Math::Matrix4& data);
	bool GetIsOrtho() const { return isOrtho; }
	void SetIsOrtho(bool value) { isOrtho = value; }
	void SetOrthoSize(double value) { orthoSize = value; }
	double GetOrthoSize() const { return orthoSize; }
	// After modifying camera position/orientation, call to rebuild the view matrix.
	void UpdateViewMatrix();
	void UpdateProjectionMatrix();
	void UploadCameraBuffer(PassConstants& mMainPassCB) const ;
	CameraRenderPath GetRenderingPath() const { return renderType; }
	template <typename Func>
	inline IPipelineResource* GetResource(void* targetComponent, const Func& func)
	{
		std::lock_guard<std::mutex> lck(mtx);
		auto ite = perCameraResource.Find(targetComponent);
		if (!ite)
		{
			IPipelineResource* newComp = func();
			perCameraResource.Insert(targetComponent, newComp);
			return newComp;
		}
		return ite.Value();
	}
	void AddAfterRenderFunction(const Runnable<void(Camera*)>& func);
private:
	Math::Matrix4 cameraToWorld;
	Math::Matrix4 mView = MathHelper::Identity4x4();
	Math::Matrix4 mProj = MathHelper::Identity4x4();
	HashMap<void*, IPipelineResource*> perCameraResource;
	std::mutex mtx;
	// Cache frustum properties.
	double mNearZ = 0.0f;
	double mFarZ = 0.0f;
	double mAspect = 0.0f;
	double mFovY = 0.0f;
	double mNearWindowHeight = 0.0f;
	double mFarWindowHeight = 0.0f;
	//Cache Ortho properties
	double orthoSize = 5;
	uint64 renderedFrame = 0;
	// Cache View/Proj matrices.
	Math::Vector3 mPosition = { 0.0f, 0.0f, 0.0f };
	Math::Vector3 mRight = { 1.0f, 0.0f, 0.0f };
	Math::Vector3 mUp = { 0.0f, 1.0f, 0.0f };
	Math::Vector3 mLook = { 0.0f, 0.0f, 1.0f };
	void ExecuteAllCallBack() noexcept;
	CameraRenderPath renderType = CameraRenderPath::DefaultPipeline;	// Camera coordinate system with coordinates relative to world space.
	vengine::vector<Runnable<void(Camera*)>> afterRenderFunction;
	bool mViewDirty = true;
	bool isOrtho = false;
	
	
};

#endif