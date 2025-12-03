#pragma once
#include "MeshComponent.h"
#include "Source/Runtime/Engine/Cloth/ClothManager.h"
#include "UClothComponent.generated.h"

UCLASS(DisplayName = "Cloth", Description = "Cloth")
class UClothComponent : public UMeshComponent
{
	GENERATED_REFLECTION_BODY()
public:
	UClothComponent();
	~UClothComponent();

	void TickComponent(float DeltaTime) override;       // 매 프레임
	void CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View) override;

	void DuplicateSubObjects() override;

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	TArray<physx::PxVec4> Particles;
	TArray<uint32_t> Indices;
	TArray<FVertexDynamic> Vertices;

	ID3D11Buffer* VertexBuffer;
	ID3D11Buffer* IndexBuffer;

	Cloth* Cloth;
	Fabric* Fabric;
	Vector<int32_t>::Type PhaseTypeInfo;

};