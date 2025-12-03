#include "pch.h"
#include "ClothManager.h"

UClothManager* UClothManager::Instance = nullptr;

UClothManager::UClothManager()
{
    Instance = this;
}
UClothManager::~UClothManager()
{
    Release();
}

void UClothManager::InitClothManager(ID3D11Device* InDevice, ID3D11DeviceContext* InContext)
{
    Device = InDevice;
    Context = InContext;
    //GraphicsContextManager = new DxContextManagerCallbackImpl(InDevice, InContext);
    // 1. 콜백 객체 생성
    if (!Allocator)
    {
        Allocator = new NvClothAllocator();
    }

    if (!ErrorCallback)
    {
        ErrorCallback = new NvClothErrorCallback();
    }

    if (!AssertHandler)
    {
        AssertHandler = new NvClothAssertHandler();
    }

    // 2. NvCloth 초기화
    nv::cloth::InitializeNvCloth(
        Allocator,
        ErrorCallback,
        AssertHandler,
        nullptr  // profiler (optional)
    );
    //Factory = NvClothCreateFactoryDX11(GraphicsContextManager);
    Factory = NvClothCreateFactoryCPU();
    Solver = Factory->createSolver();
}

void UClothManager::Release()
{
    //UE_LOG(LogTemp, Log, TEXT("Shutting down NvCloth..."));

  /*  delete GraphicsContextManager;
    GraphicsContextManager = nullptr;*/
    // Solver 정리
    if (Solver)
    {
        NV_CLOTH_DELETE(Solver);
        Solver = nullptr;
    }

    // Factory 정리
    if (Factory)
    {
        NvClothDestroyFactory(Factory);
        Factory = nullptr;
    }

    // 콜백 정리
    delete Allocator;
    delete ErrorCallback;
    delete AssertHandler;

    Allocator = nullptr;
    ErrorCallback = nullptr;
    AssertHandler = nullptr;
}
void UClothManager::Tick(float deltaTime)
{
    Solver->beginSimulation(deltaTime);
    for (int i = 0; i < Solver->getSimulationChunkCount(); i++)
    {
        Solver->simulateChunk(i);
    }
    Solver->endSimulation();
}