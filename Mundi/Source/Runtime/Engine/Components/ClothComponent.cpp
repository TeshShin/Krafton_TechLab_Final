#include "pch.h"
#include "ClothComponent.h"
#include "Source/Runtime/Engine/Cloth/ClothManager.h"
#include "D3D11RHI.h"
#include "MeshBatchElement.h"
#include "SceneView.h"

UClothComponent::UClothComponent()
{
	bCanEverTick = true;

	nv::cloth::ClothMeshDesc meshDesc;

	for (int y = 0; y <= 10; y++)
	{
		for (int x = 0; x <= 10; x++)
		{
			FVertexDynamic Vert;
			Vert.Position = FVector(x, y, 0);
			Vert.Normal = FVector(0, 0, 1);
			Vert.UV = FVector2D(x / 10.0f, y / 10.0f);
			Vertices.push_back(Vert);
			float InvMass = 1.0f;
			Particles.push_back(physx::PxVec4(Vert.Position.X, Vert.Position.Y, Vert.Position.Z, InvMass));
		}
	}

	for (int y = 0; y < 10; y++)
	{
		for (int x = 0; x < 10; x++)
		{
			int CurIdx = x + y * 11;
			Indices.push_back(CurIdx);
			Indices.push_back(CurIdx + 12);
			Indices.push_back(CurIdx + 11);
			Indices.push_back(CurIdx);
			Indices.push_back(CurIdx + 1);
			Indices.push_back(CurIdx + 12);
		}
	}

	//Fill meshDesc with data
	meshDesc.setToDefault();
	meshDesc.points.data = Vertices.data();
	meshDesc.points.stride = sizeof(FVertexDynamic);
	meshDesc.points.count = Vertices.size();

	meshDesc.triangles.data = Indices.data();
	meshDesc.triangles.stride = sizeof(uint32) * 3;
	meshDesc.triangles.count = Indices.size() / 3;
	//etc. for quads, triangles and invMasses

	physx::PxVec3 gravity(0.0f, 0.0f, -9.8f);
	Fabric = NvClothCookFabricFromMesh(UClothManager::Instance->GetFactory(), meshDesc, gravity, &PhaseTypeInfo);
	Cloth = UClothManager::Instance->GetFactory()->createCloth(GetRange(Particles), *Fabric);

	ID3D11Device* Device = UClothManager::Instance->GetDevice();
	D3D11RHI::CreateVerteBuffer(Device, Vertices, &VertexBuffer, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	D3D11RHI::CreateIndexBuffer(Device, Indices, &IndexBuffer);
	UClothManager::Instance->GetSolver()->addCloth(Cloth);

}
void UClothComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);

	MappedRange<physx::PxVec4> Particles = Cloth->getCurrentParticles();
	for (int i = 0; i < Particles.size(); i++)
	{

		PxVec3 PxPos = Particles[i].getXYZ();
		FVector Pos = FVector(PxPos.x, PxPos.y, PxPos.z);
		Vertices[i].Position = Pos;
		//do something with particles[i]
		//the xyz components are the current positions
		//the w component is the invMass.
	}
	ID3D11DeviceContext* Context = UClothManager::Instance->GetContext();

	D3D11RHI::VertexBufferUpdate(Context, VertexBuffer, Vertices);
	//destructor of particles should be called before mCloth is destroyed.
}

void UClothComponent::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
}

void UClothComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);
}
void UClothComponent::CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View)
{
	TArray<FShaderMacro> ShaderMacros = View->ViewShaderMacros;
	UShader* UberShader = UResourceManager::GetInstance().Load<UShader>("Shaders/Materials/UberLit.hlsl");
	FShaderVariant* ShaderVariant = UberShader->GetOrCompileShaderVariant(ShaderMacros);

	FMeshBatchElement BatchElement;
	if (ShaderVariant)
	{
		BatchElement.VertexShader = ShaderVariant->VertexShader;
		BatchElement.PixelShader = ShaderVariant->PixelShader;
		BatchElement.InputLayout = ShaderVariant->InputLayout;
	}
	else
	{
		BatchElement.InputLayout = UberShader->GetInputLayout();
		BatchElement.VertexShader = UberShader->GetVertexShader();
		BatchElement.PixelShader = UberShader->GetPixelShader();
	}
	BatchElement.VertexBuffer = VertexBuffer;
	BatchElement.IndexBuffer = IndexBuffer;
	BatchElement.VertexStride = sizeof(FVertexDynamic);
	BatchElement.IndexCount = Indices.size();
	BatchElement.BaseVertexIndex = 0;
	BatchElement.WorldMatrix = GetWorldMatrix();
	BatchElement.ObjectID = InternalIndex;
	BatchElement.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	OutMeshBatchElements.Add(BatchElement);
}
UClothComponent::~UClothComponent()
{

	VertexBuffer->Release();
	IndexBuffer->Release();

	Solver* Solver = UClothManager::Instance->GetSolver();
	Fabric->decRefCount();
	Solver->removeCloth(Cloth);
}