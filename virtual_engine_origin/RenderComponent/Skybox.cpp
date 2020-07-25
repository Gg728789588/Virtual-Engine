#include "Skybox.h"
#include "Mesh.h"
#include "Texture.h"
#include "Shader.h"
#include <mutex>
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "MeshRenderer.h"
#include "../Common/Camera.h"
#include "../Singleton/PSOContainer.h"
#include "DescriptorHeap.h"
#include "../LogicComponent/World.h"
#include "../Common/Camera.h"
#include "../Singleton/Graphics.h"

std::unique_ptr<Mesh> Skybox::fullScreenMesh = nullptr;
void Skybox::Draw(
	int targetPass,
	ConstBufferElement* cameraBuffer,
	const RenderPackage& package
) const
{
	UINT value = cameraBuffer->element;
	PSODescriptor desc;
	desc.meshLayoutIndex = fullScreenMesh->GetLayoutIndex();
	desc.shaderPass = targetPass;
	desc.shaderPtr = shader;
	ID3D12PipelineState* pso = package.psoContainer->GetState(desc, package.device);
	package.commandList->SetPipelineState(pso);
	shader->BindRootSignature(package.commandList, Graphics::GetGlobalDescHeap());
	shader->SetResource(package.commandList, ShaderID::GetMainTex(), Graphics::GetGlobalDescHeap(), skyboxTex->GetGlobalDescIndex());
	shader->SetResource(package.commandList, SkyboxCBufferID, cameraBuffer->buffer, cameraBuffer->element);
	package.commandList->IASetVertexBuffers(0, 1, &fullScreenMesh->VertexBufferView());
	package.commandList->IASetIndexBuffer(&fullScreenMesh->IndexBufferView());
	package.commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	package.commandList->DrawIndexedInstanced(fullScreenMesh->GetIndexCount(), 1, 0, 0, 0);
}

Skybox::~Skybox()
{
	fullScreenMesh = nullptr;
	skyboxTex = nullptr;
}

Skybox::Skybox(
	const ObjectPtr<ITexture>& tex,
	ID3D12Device* device
) : skyboxTex(tex)
{
	World* world = World::GetInstance();
	SkyboxCBufferID = ShaderID::PropertyToID("SkyboxBuffer");
	ObjectPtr<UploadBuffer> noProperty = nullptr;
	shader = ShaderCompiler::GetShader("Skybox");
	if (fullScreenMesh == nullptr) {
		std::array<DirectX::XMFLOAT3, 3> vertex;
		vertex[0] = { -3, -1, 1 };
		vertex[1] = { 1, 3, 1 };
		vertex[2] = { 1, -1, 1 };
		std::array<INT16, 3> indices{ 0, 1, 2 };
		fullScreenMesh = std::unique_ptr<Mesh>(new Mesh(
			3,
			vertex.data(),
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			device,
			DXGI_FORMAT_R16_UINT,
			3,
			indices.data()
		));
	}
}