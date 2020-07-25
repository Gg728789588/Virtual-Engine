#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "../Common/RenderPackage.h"
class PSOContainer;
class FrameResource;
class ConstBufferElement;
class Shader;
class Mesh;
class ITexture;
class Skybox : public MObject
{
private:
	static std::unique_ptr<Mesh> fullScreenMesh;
	ObjectPtr<ITexture> skyboxTex;
	const Shader* shader;
	UINT SkyboxCBufferID;
public:
	virtual ~Skybox();
	Skybox(
		const ObjectPtr<ITexture>& tex,
		ID3D12Device* device
	);

	ITexture* GetTexture() const
	{
		return skyboxTex;
	}

	void Draw(
		int targetPass,
		ConstBufferElement* cameraBuffer,
		const RenderPackage& package
	) const;
};
