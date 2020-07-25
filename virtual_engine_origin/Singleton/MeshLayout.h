#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/HashMap.h"
#include "../Common/ArrayList.h"
class MeshLayout
{
	/*
	Normal
	Tangent
	Color
	UV0
	UV2
	UV3
	UV4
	*/
private:
	static  HashMap<USHORT, UINT> layoutDict;
	static  vengine::vector<ArrayList<D3D12_INPUT_ELEMENT_DESC>*> layoutValues;
	static void GenerateDesc(
		ArrayList<D3D12_INPUT_ELEMENT_DESC>& target,
		bool normal,
		bool tangent,
		bool color,
		bool uv0,
		bool uv2,
		bool uv3,
		bool uv4,
		bool bone
	);
public:
	static ArrayList<D3D12_INPUT_ELEMENT_DESC>* GetMeshLayoutValue(UINT index);
	static UINT GetMeshLayoutIndex(
		bool normal,
		bool tangent,
		bool color,
		bool uv0,
		bool uv2,
		bool uv3,
		bool uv4,
		bool bone
	);
};