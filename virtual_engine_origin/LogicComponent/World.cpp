#include "World.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/FrameResource.h"
#include "../RenderComponent/Mesh.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "Transform.h"
#include "../CJsonObject/CJsonObject.hpp"
#include "../Common/Input.h"
#include "../Singleton/MeshLayout.h"
#include "../RenderComponent/Texture.h"
#include "../RenderComponent/MeshRenderer.h"

#include "../RenderComponent/Shader.h"
#include "../JobSystem/JobInclude.h"
#include "CameraMove.h"
#include "../ResourceManagement/AssetDatabase.h"
#include "../RenderComponent/Light.h"
#include "../Common/GameTimer.h"
#include "../LogicComponent/DirectionalLight.h"
#include "../RenderComponent/Skybox.h"
#include "../Singleton/MathLib.h"
using namespace Math;
using namespace neb;
World* World::current = nullptr;
#define MAXIMUM_RENDER_OBJECT 100000			//Take about 90M gpu memory
#define MAXIMUM_MATERIAL_OBJECT 10000			//Take about 90M gpu memory

namespace WorldTester
{
	//	ObjectPtr<Transform> testObj;
	//ObjectPtr<ITexture> testTex;
	ObjectPtr<Camera> mainCamera;
	StackObject<CameraMove> camMove;
	ObjectPtr<Transform> testLightTransform;
	ObjectPtr<Transform> sunTrans;
	DirectionalLight* sun;
}
World::World(ID3D12GraphicsCommandList* commandList, ID3D12Device* device) :
	allTransformsPtr(500)
{
	allCameras.reserve(10);
	current = this;
	WorldTester::mainCamera = ObjectPtr<Camera>::NewObject(device, Camera::CameraRenderPath::DefaultPipeline);
	allCameras.push_back(WorldTester::mainCamera);
	WorldTester::camMove.New(WorldTester::mainCamera);
	//testObj = Transform::GetTransform();
	//testObj->SetRotation(Vector4(-0.113517, 0.5999342, 0.6212156, 0.4912069));

	//meshRenderer = new MeshRenderer(trans.operator->(), device, mesh, ShaderCompiler::GetShader("OpaqueStandard"));
	WorldTester::testLightTransform = Transform::GetTransform();
	testLight = Light::CreateLight(WorldTester::testLightTransform);
	testLight->SetLightType(Light::LightType_Spot);
	testLight->SetRange(10);
	testLight->SetIntensity(250);
	testLight->SetShadowNearPlane(0.2f);
	testLight->SetSpotAngle(90);
	testLight->SetSpotSmallAngle(10);
	testLight->SetEnabled(true);
	testLight->SetShadowEnabled(true, device);
	WorldTester::sunTrans = Transform::GetTransform();
	WorldTester::sunTrans->SetRotation({ -0.0004715632f,0.9983955f,0.008083393f,0.05604406f });

	uint4 shadowRes = { 2048,2048,2048,2048 };
	WorldTester::sun = DirectionalLight::GetInstance(WorldTester::sunTrans, (uint*)&shadowRes, device);
	WorldTester::sun->intensity = 2;
	for (uint i = 0; i < 4; ++i)
	{
		WorldTester::sun->shadowSoftValue[i] *= 2;
	}
	ObjectPtr<Texture> skyboxTex = ObjectPtr< Texture>::NewObject(
		device,
		"Resource/Sky.vtex",
		true,
		TextureDimension::Cubemap
	);
	ObjectPtr<ITexture> it = skyboxTex.CastTo<ITexture>();
	currentSkybox = ObjectPtr<Skybox>::NewObject(
		it,
		device
	);
}

void World::DestroyAllCameras()
{

	for (auto ite = allCameras.begin(); ite != allCameras.end(); ++ite)
	{
		ite->Destroy();
	}
	allCameras.clear();
}

World::~World()
{
	for (uint i = 0; i < allTransformsPtr.Length(); ++i)
	{
		Transform::DisposeTransform(allTransformsPtr[i]);
	}
	DestroyAllCameras();
	WorldTester::camMove.Delete();
	currentSkybox.Destroy();
}
#include "../PipelineComponent/RenderPipeline.h"
void World::Update(FrameResource* resource, ID3D12Device* device, GameTimer& timer, uint64 frameIndex, int2 screenSize, const Math::Vector3& moveDir)
{
	WorldTester::camMove->Run(timer.DeltaTime());
	WorldTester::mainCamera->SetLens(0.333333 * MathHelper::Pi, (float)screenSize.x / (float)screenSize.y, 0.3, 1000);
	/*if (!testTex)
	{
		testTex = ObjectPtr<ITexture>::MakePtr(new Texture(device, "Resource/testTex.vtex", TextureDimension::Tex2D, 20, 0));
		pbrMat->SetEmissionTexture(testTex.CastTo<ITexture>());
		pbrMat->UpdateMaterialToBuffer();

	}*/
	if (Input::IsKeyPressed(KeyCode::Ctrl))
	{
		WorldTester::sunTrans->SetForward(-WorldTester::mainCamera->GetLook());
		WorldTester::sunTrans->SetUp(WorldTester::mainCamera->GetUp());
		WorldTester::sunTrans->SetRight(-WorldTester::mainCamera->GetRight());
	}
	
	if (Input::IsKeyDown(KeyCode::Space))
	{
		WorldTester::testLightTransform->SetRight(WorldTester::mainCamera->GetRight());
		WorldTester::testLightTransform->SetUp(WorldTester::mainCamera->GetUp());
		WorldTester::testLightTransform->SetForward(WorldTester::mainCamera->GetLook());
		WorldTester::testLightTransform->SetPosition(WorldTester::mainCamera->GetPosition());
	}
	for (auto ite = allCameras.begin(); ite != allCameras.end(); ++ite)
	{
		(*ite)->UpdateViewMatrix();
		(*ite)->UpdateProjectionMatrix();
	}
}

#undef MAXIMUM_RENDER_OBJECT