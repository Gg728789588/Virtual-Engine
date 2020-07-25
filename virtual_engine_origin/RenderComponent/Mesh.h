#pragma once
#include "IGPUResource.h"
class FrameResource;
class Mesh final : public IGPUResource
{
	// Data about the buffers.
	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	UINT meshLayoutIndex;
	UINT mVertexCount;
	DXGI_FORMAT indexFormat;
	UINT indexCount;
	void* indexArrayPtr;
	D3D12_VERTEX_BUFFER_VIEW vertexView;
	D3D12_INDEX_BUFFER_VIEW indexView;
	static uint64_t GetStride(
		bool positions,
		bool normals,
		bool tangents,
		bool colors,
		bool uv,
		bool uv1,
		bool uv2,
		bool uv3,
		bool boneIndex,
		bool boneWeight);
public:
	static uint64_t GetMeshSize(
		uint vertexCount,
		bool positions,
		bool normals,
		bool tangents,
		bool colors,
		bool uv,
		bool uv1,
		bool uv2,
		bool uv3,
		bool boneIndex,
		bool boneWeight,
		uint indexCount,
		DXGI_FORMAT indexFormat);
	UINT GetIndexCount() const { return indexCount; }
	UINT GetIndexFormat() const { return indexFormat; }
	float3 boundingCenter = { 0,0,0 };
	float3 boundingExtent = { 0.5f,0.5f,0.5f };
	inline UINT GetLayoutIndex() const { return meshLayoutIndex; }
	inline UINT GetVertexCount() const { return mVertexCount; }
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const { return vertexView; }
	D3D12_INDEX_BUFFER_VIEW IndexBufferView() const { return indexView; }
	Mesh(
		int vertexCount,
		float3* positions,
		float3* normals,
		float4* tangents,
		float4* colors,
		float2* uv,
		float2* uv1,
		float2* uv2,
		float2* uv3,
		int4* boneIndex,
		float4* boneWeight,
		ID3D12Device* device,
		DXGI_FORMAT indexFormat,
		UINT indexCount,
		void* indexArrayPtr,
		const Microsoft::WRL::ComPtr<ID3D12Resource>& defaultBuffer = nullptr,
		uint64_t defaultOffset = 0,
		const Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer = nullptr,
		uint64_t uploadOffset = 0
	);

	static Mesh* LoadMeshFromFile(const vengine::string& str, ID3D12Device* device,
		bool normals,
		bool tangents,
		bool colors,
		bool uv,
		bool uv1,
		bool uv2,
		bool uv3,
		bool bone);
	static void LoadMeshFromFiles(const vengine::vector<vengine::string>& str, ID3D12Device* device,
		bool normals,
		bool tangents,
		bool colors,
		bool uv,
		bool uv1,
		bool uv2,
		bool uv3,
		bool bone,
		vengine::vector<ObjectPtr<Mesh>>& results);
};