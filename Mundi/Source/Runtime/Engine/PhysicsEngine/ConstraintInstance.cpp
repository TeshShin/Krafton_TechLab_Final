#include "pch.h"
#include "ConstraintInstance.h"
#include "BodyInstance.h"
#include "PhysScene.h"

FConstraintInstance::FConstraintInstance()
    : Joint(nullptr)
    , PhysScene(nullptr)
    , ParentBodyInstance(nullptr)
    , ChildBodyInstance(nullptr)
{
}

FConstraintInstance::~FConstraintInstance()
{
    TermConstraint();
}

void FConstraintInstance::InitConstraint(
    const FConstraintSetup& Setup,
    FBodyInstance* ParentBody,
    FBodyInstance* ChildBody,
    FPhysScene* InPhysScene)
{
    if (!GPhysXSDK)
    {
        UE_LOG("[PhysX Error] InitConstraint: GPhysXSDK is null");
        return;
    }

    // 기존 Joint 정리
    if (Joint)
    {
        TermConstraint();
    }

    // 최소 하나의 바디가 필요
    if (!ParentBody && !ChildBody)
    {
        UE_LOG("[PhysX Error] InitConstraint: Both bodies are null");
        return;
    }

    PhysScene = InPhysScene;
    ParentBodyInstance = ParentBody;
    ChildBodyInstance = ChildBody;

    // PhysX Actor 가져오기 (nullptr 허용 - 월드에 고정)
    PxRigidActor* ParentActor = ParentBody ? ParentBody->RigidActor : nullptr;
    PxRigidActor* ChildActor = ChildBody ? ChildBody->RigidActor : nullptr;

    if (!ParentActor && !ChildActor)
    {
        UE_LOG("[PhysX Error] InitConstraint: No valid RigidActor");
        return;
    }

    // 로컬 프레임 계산: 조인트 위치는 자식 바디의 원점 (본의 시작점)
    // 부모 바디 기준: 자식 본 위치를 부모 본 로컬 좌표로 변환
    // 자식 바디 기준: 원점 (자기 자신의 시작점)
    PxTransform ParentLocalFrame = PxTransform(PxIdentity);
    PxTransform ChildLocalFrame = PxTransform(PxIdentity);

    if (ParentActor && ChildActor)
    {
        // 두 바디의 월드 트랜스폼 가져오기
        PxTransform ParentWorldPose = ParentActor->getGlobalPose();
        PxTransform ChildWorldPose = ChildActor->getGlobalPose();

        // 조인트 위치 = 자식 바디의 월드 위치 (본의 시작점)
        PxVec3 JointWorldPos = ChildWorldPose.p;

        // 부모 로컬 프레임: 조인트 위치를 부모 바디 로컬 좌표로 변환
        PxTransform ParentWorldInv = ParentWorldPose.getInverse();
        PxVec3 JointInParentLocal = ParentWorldInv.transform(JointWorldPos);
        ParentLocalFrame = PxTransform(JointInParentLocal);

        // 자식 로컬 프레임: 자식 바디 원점 (0,0,0)
        ChildLocalFrame = PxTransform(PxIdentity);
    }

    // PxD6Joint 생성
    Joint = PxD6JointCreate(
        *GPhysXSDK,
        ParentActor, ParentLocalFrame,
        ChildActor, ChildLocalFrame
    );

    if (!Joint)
    {
        UE_LOG("[PhysX Error] InitConstraint: Failed to create PxD6Joint");
        return;
    }

    // 제약 조건 설정 적용
    ConfigureJointMotion(Setup);
    ConfigureJointLimits(Setup);
    ConfigureJointDrive(Setup);

    UE_LOG("[PhysX] Constraint '%s' created successfully", Setup.JointName.ToString().c_str());
}

void FConstraintInstance::TermConstraint()
{
    if (Joint)
    {
        Joint->release();
        Joint = nullptr;
    }

    PhysScene = nullptr;
    ParentBodyInstance = nullptr;
    ChildBodyInstance = nullptr;
}

void FConstraintInstance::UpdateFromSetup(const FConstraintSetup& Setup)
{
    if (!Joint)
    {
        return;
    }

    ConfigureJointMotion(Setup);
    ConfigureJointLimits(Setup);
    ConfigureJointDrive(Setup);
}

void FConstraintInstance::ConfigureJointMotion(const FConstraintSetup& Setup)
{
    if (!Joint)
    {
        return;
    }

    // 기본: 모든 선형 축 잠금 (위치 고정)
    Joint->setMotion(PxD6Axis::eX, PxD6Motion::eLOCKED);
    Joint->setMotion(PxD6Axis::eY, PxD6Motion::eLOCKED);
    Joint->setMotion(PxD6Axis::eZ, PxD6Motion::eLOCKED);

    switch (Setup.ConstraintType)
    {
    case EConstraintType::BallAndSocket:
        // 구형 조인트: 모든 회전축 제한된 자유도
        Joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLIMITED);
        Joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
        Joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);
        break;

    case EConstraintType::Hinge:
        // 힌지 조인트: Twist(X축 회전)만 제한된 자유도, 나머지 잠금
        Joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLIMITED);
        Joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLOCKED);
        Joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLOCKED);
        break;
    }
}

void FConstraintInstance::ConfigureJointLimits(const FConstraintSetup& Setup)
{
    if (!Joint)
    {
        return;
    }

    // 각도를 라디안으로 변환 (PhysX는 최소 0.01 라디안 필요)
    const float MinAngle = 0.01f;  // 약 0.57도
    float Swing1Rad = FMath::Max(MinAngle, DegreesToRadians(Setup.Swing1Limit));
    float Swing2Rad = FMath::Max(MinAngle, DegreesToRadians(Setup.Swing2Limit));
    float TwistMinRad = DegreesToRadians(Setup.TwistLimitMin);
    float TwistMaxRad = DegreesToRadians(Setup.TwistLimitMax);

    // Twist 범위가 너무 작으면 최소 범위 보장
    if (TwistMaxRad - TwistMinRad < MinAngle)
    {
        TwistMaxRad = TwistMinRad + MinAngle;
    }

    // Swing 제한 (Cone Limit)
    // PxJointLimitCone(yAngle, zAngle, contactDistance)
    PxJointLimitCone SwingLimit(Swing1Rad, Swing2Rad, 0.01f);
    SwingLimit.stiffness = Setup.Stiffness;
    SwingLimit.damping = Setup.Damping;
    Joint->setSwingLimit(SwingLimit);

    // Twist 제한 (Angular Pair Limit)
    // PxJointAngularLimitPair(lowerLimit, upperLimit, contactDistance)
    PxJointAngularLimitPair TwistLimit(TwistMinRad, TwistMaxRad, 0.01f);
    TwistLimit.stiffness = Setup.Stiffness;
    TwistLimit.damping = Setup.Damping;
    Joint->setTwistLimit(TwistLimit);
}

void FConstraintInstance::ConfigureJointDrive(const FConstraintSetup& Setup)
{
    if (!Joint)
    {
        return;
    }

    // 스프링 드라이브 설정 (Stiffness > 0 인 경우만)
    if (Setup.Stiffness > 0.0f || Setup.Damping > 0.0f)
    {
        PxD6JointDrive Drive(Setup.Stiffness, Setup.Damping, PX_MAX_F32, false);

        // 각 회전축에 드라이브 적용
        Joint->setDrive(PxD6Drive::eSWING, Drive);
        Joint->setDrive(PxD6Drive::eTWIST, Drive);
    }
}
