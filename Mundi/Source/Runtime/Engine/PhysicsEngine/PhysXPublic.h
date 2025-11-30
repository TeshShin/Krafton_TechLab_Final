#pragma once
#include <PxPhysicsAPI.h>

using namespace physx;

// ==================================================================================
// SCENE LOCK
// ==================================================================================

/**
 * @note NULL 체크를 위해서 PxSceneReadLock 대신에 이 RAII 클래스를 활용한다.
 */
class FPhysXSceneReadLock
{
public:
    FPhysXSceneReadLock(PxScene* PInScene, const char* filename, PxU32 lineno)
        : PScene(PInScene)
    {
        if (PScene)
        {
            PScene->lockRead(filename, lineno);
        }
    }

    ~FPhysXSceneReadLock()
    {
        if (PScene)
        {
            PScene->unlockRead();
        }
    }

private:
    PxScene* PScene;
};

/**
 * @note NULL 체크를 위해서 PxSceneWriteLock 대신에 이 RAII 클래스를 활용한다.
 */
class FPhysXSceneWriteLock
{
public:
    FPhysXSceneWriteLock(PxScene* PInScene, const char* filename, PxU32 lineno)
        : PScene(PInScene)
    {
        if (PScene)
        {
            PScene->lockWrite(filename, lineno);
        }
    }

    ~FPhysXSceneWriteLock()
    {
        if (PScene)
        {
            PScene->unlockWrite();
        }
    }

private:
    PxScene* PScene;
};

#define PREPROCESSOR_JOIN(x, y) PREPROCESSOR_JOIN_INNER(x, y)
#define PREPROCESSOR_JOIN_INNER(x, y) x##y

#define SCOPED_SCENE_READ_LOCK( _scene ) FPhysXSceneReadLock PREPROCESSOR_JOIN(_rlock, __LINE__)(_scene, __FILE__, __LINE__)
#define SCOPED_SCENE_WRITE_LOCK( _scene ) FPhysXSceneWriteLock PREPROCESSOR_JOIN(_wlock, __LINE__)(_scene, __FILE__, __LINE__)

// ==================================================================================
// BASIC TYPE CONVERSIONS
// ==================================================================================

inline PxVec3 U2PVector(const FVector& UVec)
{
    return PxVec3(UVec.X, UVec.Y, UVec.Z); 
}

inline PxVec4 U2PVector(const FVector4& UVec)
{
    return PxVec4(UVec.X, UVec.Y, UVec.Z, UVec.W);
}

inline PxQuat U2PQuat(const FQuat& UQuat)
{
    return PxQuat(UQuat.X, UQuat.Y, UQuat.Z, UQuat.W);
}

inline PxTransform U2PTransform(const FTransform& UTransform)
{
    return PxTransform(U2PVector(UTransform.Translation), U2PQuat(UTransform.Rotation));
}

/**
 * @note PhysX는 right-multiplication & column-major를 사용하고, 퓨처 엔진은 left-multiplication & row-major를 사용한다.
 *       따라서, 두 행렬은 메모리 상에서 동일하게 저장되므로 별도의 행렬 변환이 필요하지 않다.
 */
inline PxMat44 U2PMatrix(const FMatrix& UMat)
{
    PxMat44 Result;
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            Result[r][c] = UMat.M[r][c];
        }
    }
    return Result;
}

inline FVector P2UVector(const PxVec3& PVec)
{
    return FVector(PVec.x, PVec.y, PVec.z);
}

inline FVector4 P2UVector(const PxVec4& PVec)
{
    return FVector4(PVec.x, PVec.y, PVec.z, PVec.w);
}

inline FQuat P2UQuat(const PxQuat& PQuat)
{
    return FQuat(PQuat.x, PQuat.y, PQuat.z, PQuat.w);
}

inline FTransform P2UTransform(const PxTransform& PTransform)
{
    return FTransform(
        P2UVector(PTransform.p),           // Translation
        P2UQuat(PTransform.q),             // Rotation
        FVector(1.f, 1.f, 1.f) // Scale (PhysX는 스케일 정보 없음)
    );
}

inline FMatrix P2UMatrix(const PxMat44& PMat)
{
    FMatrix Result;
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            Result.M[r][c] = PMat[r][c];
        }
    }
    return Result;
}

// ==================================================================================
