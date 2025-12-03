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

    // 저장된 Frame 데이터를 사용하여 로컬 프레임 생성
    PxTransform ChildLocalFrame = ConvertToPxTransform(Setup.ChildPosition, Setup.ChildRotation);
    PxTransform ParentLocalFrame = ConvertToPxTransform(Setup.ParentPosition, Setup.ParentRotation);

    // AngularRotationOffset 적용 (Parent Frame에만)
    if (!Setup.AngularRotationOffset.IsZero())
    {
        ParentLocalFrame.q = ApplyAngularOffset(ParentLocalFrame.q, Setup.AngularRotationOffset);
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

    // 디버그: 생성 직후 각도 확인
    float InitTwist, InitSwing1, InitSwing2;
    GetCurrentAngles(InitTwist, InitSwing1, InitSwing2);

    UE_LOG("[PhysX] Constraint '%s' created: InitAngles(Twist=%.1f, Swing1=%.1f, Swing2=%.1f)",
        Setup.JointName.ToString().c_str(), InitTwist, InitSwing1, InitSwing2);
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

namespace
{
    // ELinearConstraintMotion -> PxD6Motion 변환
    PxD6Motion::Enum ToPxD6Motion(ELinearConstraintMotion Motion)
    {
        switch (Motion)
        {
        case ELinearConstraintMotion::Free:    return PxD6Motion::eFREE;
        case ELinearConstraintMotion::Limited: return PxD6Motion::eLIMITED;
        case ELinearConstraintMotion::Locked:  return PxD6Motion::eLOCKED;
        default:                               return PxD6Motion::eLOCKED;
        }
    }

    // EAngularConstraintMotion -> PxD6Motion 변환
    PxD6Motion::Enum ToPxD6Motion(EAngularConstraintMotion Motion)
    {
        switch (Motion)
        {
        case EAngularConstraintMotion::Free:    return PxD6Motion::eFREE;
        case EAngularConstraintMotion::Limited: return PxD6Motion::eLIMITED;
        case EAngularConstraintMotion::Locked:  return PxD6Motion::eLOCKED;
        default:                                return PxD6Motion::eLOCKED;
        }
    }
}

void FConstraintInstance::ConfigureJointMotion(const FConstraintSetup& Setup)
{
    if (!Joint)
    {
        return;
    }

    // Linear 축 설정
    Joint->setMotion(PxD6Axis::eX, ToPxD6Motion(Setup.LinearXMotion));
    Joint->setMotion(PxD6Axis::eY, ToPxD6Motion(Setup.LinearYMotion));
    Joint->setMotion(PxD6Axis::eZ, ToPxD6Motion(Setup.LinearZMotion));

    // Angular 축 설정
    Joint->setMotion(PxD6Axis::eTWIST,  ToPxD6Motion(Setup.TwistMotion));
    Joint->setMotion(PxD6Axis::eSWING1, ToPxD6Motion(Setup.Swing1Motion));
    Joint->setMotion(PxD6Axis::eSWING2, ToPxD6Motion(Setup.Swing2Motion));

    UE_LOG("[Joint] ConfigureJointMotion: Linear(%s,%s,%s) Angular(Twist=%s,Swing1=%s,Swing2=%s)",
        GetLinearConstraintMotionName(Setup.LinearXMotion),
        GetLinearConstraintMotionName(Setup.LinearYMotion),
        GetLinearConstraintMotionName(Setup.LinearZMotion),
        GetAngularConstraintMotionName(Setup.TwistMotion),
        GetAngularConstraintMotionName(Setup.Swing1Motion),
        GetAngularConstraintMotionName(Setup.Swing2Motion));
}

void FConstraintInstance::ConfigureJointLimits(const FConstraintSetup& Setup)
{
    if (!Joint)
    {
        return;
    }

    // ─────────────────────────────────────────────────────────────
    // Linear Limit (LIMITED인 축이 있을 때만)
    // ─────────────────────────────────────────────────────────────
    if (Setup.HasLinearLimit())
    {
        // PhysX는 cm 단위 사용 (Unreal과 동일)
        PxJointLinearLimit LinearLimit(GPhysXSDK->getTolerancesScale(), Setup.LinearLimit, 0.05f);
        LinearLimit.stiffness = 0.0f;
        LinearLimit.damping = 0.0f;
        LinearLimit.restitution = 0.0f;
        Joint->setLinearLimit(LinearLimit);
    }

    // ─────────────────────────────────────────────────────────────
    // Angular Limit - Swing (Cone)
    // ─────────────────────────────────────────────────────────────
    if (Setup.Swing1Motion == EAngularConstraintMotion::Limited ||
        Setup.Swing2Motion == EAngularConstraintMotion::Limited)
    {
        float Swing1Rad = DegreesToRadians(Setup.Swing1LimitDegrees);
        float Swing2Rad = DegreesToRadians(Setup.Swing2LimitDegrees);

        PxJointLimitCone SwingLimit(Swing1Rad, Swing2Rad, 0.05f);
        SwingLimit.stiffness = 0.0f;
        SwingLimit.damping = 0.0f;
        SwingLimit.restitution = 0.0f;
        Joint->setSwingLimit(SwingLimit);
    }

    // ─────────────────────────────────────────────────────────────
    // Angular Limit - Twist (대칭: ±TwistLimitDegrees)
    // ─────────────────────────────────────────────────────────────
    if (Setup.TwistMotion == EAngularConstraintMotion::Limited)
    {
        float TwistRad = DegreesToRadians(Setup.TwistLimitDegrees);

        PxJointAngularLimitPair TwistLimit(-TwistRad, TwistRad, 0.05f);
        TwistLimit.stiffness = 0.0f;
        TwistLimit.damping = 0.0f;
        TwistLimit.restitution = 0.0f;
        Joint->setTwistLimit(TwistLimit);
    }

    // 프로젝션 활성화 - 제한 위반 시 강제 보정
    Joint->setProjectionLinearTolerance(0.1f);
    Joint->setProjectionAngularTolerance(DegreesToRadians(5.0f));
    Joint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
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

void FConstraintInstance::GetCurrentAngles(float& OutTwist, float& OutSwing1, float& OutSwing2) const
{
    OutTwist = 0.0f;
    OutSwing1 = 0.0f;
    OutSwing2 = 0.0f;

    if (!Joint)
    {
        return;
    }

    // PhysX D6Joint에서 직접 각도 가져오기
    OutTwist = RadiansToDegrees(Joint->getTwistAngle());
    OutSwing1 = RadiansToDegrees(Joint->getSwingYAngle());
    OutSwing2 = RadiansToDegrees(Joint->getSwingZAngle());
}

void FConstraintInstance::LogCurrentAngles(const FName& JointName) const
{
    float Twist, Swing1, Swing2;
    GetCurrentAngles(Twist, Swing1, Swing2);

    UE_LOG("[Joint] %s: Twist=%.1f, Swing1=%.1f, Swing2=%.1f (deg)",
        JointName.ToString().c_str(), Twist, Swing1, Swing2);
}

// ─────────────────────────────────────────────────────────────────────────────
// Static Helper Functions
// ─────────────────────────────────────────────────────────────────────────────

PxTransform FConstraintInstance::ConvertToPxTransform(
    const FVector& Position,
    const FVector& RotationDegrees)
{
    // Position 변환
    PxVec3 PxPos(Position.X, Position.Y, Position.Z);

    // Euler (Roll, Pitch, Yaw) -> Quaternion
    // FVector의 X=Roll, Y=Pitch, Z=Yaw로 해석
    float RollRad = DegreesToRadians(RotationDegrees.X);
    float PitchRad = DegreesToRadians(RotationDegrees.Y);
    float YawRad = DegreesToRadians(RotationDegrees.Z);

    // ZYX 순서로 회전 적용 (Yaw -> Pitch -> Roll)
    PxQuat QYaw(YawRad, PxVec3(0, 0, 1));
    PxQuat QPitch(PitchRad, PxVec3(0, 1, 0));
    PxQuat QRoll(RollRad, PxVec3(1, 0, 0));

    PxQuat PxRot = QYaw * QPitch * QRoll;
    PxRot.normalize();

    return PxTransform(PxPos, PxRot);
}

PxQuat FConstraintInstance::ApplyAngularOffset(
    const PxQuat& BaseRotation,
    const FVector& OffsetDegrees)
{
    // Offset Euler -> Quaternion
    float RollRad = DegreesToRadians(OffsetDegrees.X);
    float PitchRad = DegreesToRadians(OffsetDegrees.Y);
    float YawRad = DegreesToRadians(OffsetDegrees.Z);

    PxQuat QYaw(YawRad, PxVec3(0, 0, 1));
    PxQuat QPitch(PitchRad, PxVec3(0, 1, 0));
    PxQuat QRoll(RollRad, PxVec3(1, 0, 0));

    PxQuat OffsetQuat = QYaw * QPitch * QRoll;
    OffsetQuat.normalize();

    // Base 회전에 Offset 적용
    PxQuat Result = BaseRotation * OffsetQuat;
    Result.normalize();

    return Result;
}

void FConstraintInstance::CalculateFramesFromBones(
    FConstraintSetup& OutSetup,
    FBodyInstance* ParentBody,
    FBodyInstance* ChildBody)
{
    if (!ParentBody || !ChildBody)
    {
        UE_LOG("[Constraint] CalculateFramesFromBones: Invalid bodies");
        return;
    }

    if (!ParentBody->RigidActor || !ChildBody->RigidActor)
    {
        UE_LOG("[Constraint] CalculateFramesFromBones: No valid RigidActors");
        return;
    }

    // 월드 트랜스폼 가져오기
    PxTransform ParentWorldPose = ParentBody->RigidActor->getGlobalPose();
    PxTransform ChildWorldPose = ChildBody->RigidActor->getGlobalPose();

    // 조인트 위치 = 자식 바디의 월드 위치 (본의 시작점)
    PxVec3 JointWorldPos = ChildWorldPose.p;

    // ─────────────────────────────────────────────────────────────
    // Child Frame: 원점, Identity 회전 (자식 바디 로컬 기준)
    // ─────────────────────────────────────────────────────────────
    OutSetup.ChildPosition = FVector::Zero();
    OutSetup.ChildRotation = FVector::Zero();

    // ─────────────────────────────────────────────────────────────
    // Parent Frame: 조인트 위치를 부모 로컬로 변환
    // ─────────────────────────────────────────────────────────────
    PxTransform ParentWorldInv = ParentWorldPose.getInverse();
    PxVec3 JointInParentLocal = ParentWorldInv.transform(JointWorldPos);

    OutSetup.ParentPosition = FVector(JointInParentLocal.x, JointInParentLocal.y, JointInParentLocal.z);

    // 부모와 자식의 상대 회전 계산
    PxQuat RelativeRot = ParentWorldPose.q.getConjugate() * ChildWorldPose.q;
    RelativeRot.normalize();

    // Quaternion -> Euler 변환
    // PhysX PxQuat에서 Euler 각도 추출
    float Roll, Pitch, Yaw;

    // toRadiansAndUnitAxis 대신 직접 Euler 변환
    // 참고: https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
    float qw = RelativeRot.w;
    float qx = RelativeRot.x;
    float qy = RelativeRot.y;
    float qz = RelativeRot.z;

    // Roll (X축)
    float sinr_cosp = 2.0f * (qw * qx + qy * qz);
    float cosr_cosp = 1.0f - 2.0f * (qx * qx + qy * qy);
    Roll = atan2f(sinr_cosp, cosr_cosp);

    // Pitch (Y축)
    float sinp = 2.0f * (qw * qy - qz * qx);
    if (fabsf(sinp) >= 1.0f)
        Pitch = copysignf(PxPi / 2.0f, sinp);  // Gimbal lock
    else
        Pitch = asinf(sinp);

    // Yaw (Z축)
    float siny_cosp = 2.0f * (qw * qz + qx * qy);
    float cosy_cosp = 1.0f - 2.0f * (qy * qy + qz * qz);
    Yaw = atan2f(siny_cosp, cosy_cosp);

    OutSetup.ParentRotation = FVector(
        RadiansToDegrees(Roll),
        RadiansToDegrees(Pitch),
        RadiansToDegrees(Yaw)
    );

    // AngularRotationOffset 초기값은 Zero
    OutSetup.AngularRotationOffset = FVector::Zero();

    UE_LOG("[Constraint] CalculateFramesFromBones: ParentPos=(%.2f, %.2f, %.2f) ParentRot=(%.1f, %.1f, %.1f)",
        OutSetup.ParentPosition.X, OutSetup.ParentPosition.Y, OutSetup.ParentPosition.Z,
        OutSetup.ParentRotation.X, OutSetup.ParentRotation.Y, OutSetup.ParentRotation.Z);
}
