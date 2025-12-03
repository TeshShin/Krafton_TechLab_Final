#pragma once
#include "Object.h"
#include "d3d11.h"
#include <NvCloth/Factory.h>
#include <NvCloth/Solver.h>
#include <NvCloth/Cloth.h>
#include <NvCloth/Fabric.h>
#include <foundation/PxVec3.h>
#include <foundation/PxVec4.h>
#include <NvCloth/extensions/ClothFabricCooker.h> 
#include <NvCloth/DxContextManagerCallback.h>
#include "DxContextManagerCallbackImpl.h"
#include "NvClothEnvironment.h"

using namespace nv::cloth;
using namespace physx;

class UClothManager
{
public:
    UClothManager();
    ~UClothManager();
    static UClothManager* Instance;
	void InitClothManager(ID3D11Device* InDevice, ID3D11DeviceContext* InContext);
	void Tick(float deltaTime);
    void Release();
    
    Factory* GetFactory() { return Factory; }
    Solver* GetSolver() { return Solver; }

    //임시
    ID3D11Device* GetDevice() { return Device; }
    ID3D11DeviceContext* GetContext() { return Context; }


private:
    DxContextManagerCallback* GraphicsContextManager = nullptr;
    Factory* Factory = nullptr;
    Solver* Solver = nullptr;

    ID3D11Device* Device;
    ID3D11DeviceContext* Context;

    // NvCloth 초기화에 필요한 콜백들
    NvClothAllocator* Allocator = nullptr;
    NvClothErrorCallback* ErrorCallback = nullptr;
    NvClothAssertHandler* AssertHandler = nullptr;

}; 
inline Range<physx::PxVec4> GetRange(TArray<PxVec4>& Arr)
{
    return Range<physx::PxVec4>(
        Arr.data(),
        Arr.data() + Arr.size());
}