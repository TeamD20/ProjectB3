// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PBDiceRollActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;

// 굴림 완료 시 결과 숫자를 전달하는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDiceRollFinished, int32, ResultNumber);

/** 주사위 굴림 연출 페이즈 */
UENUM()
enum class EPBDiceRollPhase : uint8
{
    // 대기 상태
    Idle,
    // 삼각함수 기반 랜덤 굴림 중
    Rolling,
    // 목표 면으로 수렴 중
    Settling,
};

/**
 * D20 주사위 굴림 연출 액터.
 */
UCLASS()
class PROJECTB3_API APBDiceRollActor : public AActor
{
    GENERATED_BODY()

public:
    APBDiceRollActor();

    /*~ AActor Interface ~*/
    virtual void Tick(float DeltaTime) override;

    /*~ APBDiceRollActor Interface ~*/
    /**
     * 지정한 면의 법선이 액터의 Forward(+X) 방향을 바라보도록 메시를 회전.
     * BP Construction Script에서 FaceIndex를 바꿔가며 호출하여 면-숫자 매핑을 확인한다.
     * @param FaceIndex - 정렬할 면 인덱스 (0~19)
     */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Dice")
    void AlignFaceToForward(int32 FaceIndex);

    /**
     * 지정한 주사위 숫자(1~20)에 해당하는 면이 Forward(+X)를 바라보도록 메시를 회전.
     * @param DiceNumber - 정면으로 향할 주사위 숫자 (1~20). 매핑에 없으면 무시.
     */
    UFUNCTION(BlueprintCallable, Category = "Dice")
    void AlignDiceNumberToForward(int32 DiceNumber);

    /**
     * 텀블링 연출.
     * 완료 시 OnDiceRollFinished를 브로드캐스트
     * @param TargetNumber   - 최종적으로 정면에 올 주사위 숫자 (외부에서 미리 결정된 값)
     * @param RollDuration   - 굴림 지속 시간 (초)
     * @param SettleDuration - 목표 면으로 수렴하는 시간 (초)
     */
    UFUNCTION(BlueprintCallable, Category = "Dice")
    void RollToNumber(int32 TargetNumber, float RollDuration = 1.2f, float SettleDuration = 0.3f);

    /**
     * 현재 메시 회전 기준으로 가장 Forward(+X)에 가까운 면 인덱스를 반환.
     */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Dice")
    int32 GetFaceIndexFacingForward() const;

    /**
     * 현재 Forward(+X)에 가장 가까운 면의 주사위 숫자를 반환.
     * FaceIndexToNumber 매핑이 완성된 후 사용. 매핑에 없으면 -1 반환.
     */
    UFUNCTION(BlueprintCallable, Category = "Dice")
    int32 GetDiceNumberFacingForward() const;

    /** D20FaceNormals 배열의 특정 인덱스 법선을 월드 스페이스로 반환 (디버그용) */
    UFUNCTION(BlueprintCallable, Category = "Dice")
    FVector GetFaceNormalInWorldSpace(int32 FaceIndex) const;

private:
    /** 지정 숫자에 해당하는 목표 Quat 계산. 매핑에 없으면 FQuat::Identity 반환 */
    FQuat CalcTargetQuat(int32 DiceNumber) const;

    /** Tick에서 Rolling 페이즈 처리 */
    void TickRolling(float DeltaTime);

    /** Tick에서 Settling 페이즈 처리 */
    void TickSettling(float DeltaTime);
    
public:
    // 굴림 완료 이벤트. ResultNumber: 최종 정면에 온 주사위 숫자
    UPROPERTY(BlueprintAssignable, Category = "Dice")
    FOnDiceRollFinished OnDiceRollFinished;

    // 메시 초기 배치 오프셋.
    // 수학적 표준 배치와 실제 메시 회전이 다를 경우 이 값으로 보정한다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice")
    FRotator MeshRotationOffset = FRotator::ZeroRotator;

    // Construction Script에서 정렬할 면 인덱스 (0~19).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice")
    int32 PreviewFaceIndex = 0;

    // 면 인덱스(0~19) : 주사위 숫자(1~20) 매핑 테이블.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice")
    TMap<int32, int32> FaceIndexToNumber;

    // 굴림 시 각 축에 부여할 각속도 범위 최솟값 (deg/s)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice|Roll", meta = (ClampMin = "1.0"))
    float MinAngularSpeed = 360.f;

    // 굴림 시 각 축에 부여할 각속도 범위 최댓값 (deg/s)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice|Roll", meta = (ClampMin = "1.0"))
    float MaxAngularSpeed = 720.f;

    // 목표 각속도를 새로 뽑는 주기 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice|Roll", meta = (ClampMin = "0.1"))
    float VelocityChangeInterval = 0.4f;

    // 새 목표 각속도로 보간하는 속도. 클수록 빠르게 전환
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice|Roll", meta = (ClampMin = "0.1"))
    float VelocityInterpSpeed = 3.f;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> DiceMesh;

private:
    /**
     * 수학적으로 계산된 정이십면체 20개 면의 단위 법선 벡터 배열.
     * 인덱스 0~19, 표준 배치(황금비 기반 꼭짓점) 기준.
     */
    static const TArray<FVector> D20FaceNormals;

    // 현재 굴림 페이즈
    EPBDiceRollPhase RollPhase = EPBDiceRollPhase::Idle;

    // Rolling 페이즈 경과 시간 (초)
    float RollingElapsed = 0.f;

    // Rolling 페이즈 전체 시간 (초)
    float RollingDuration = 0.f;

    // Settling 페이즈 전체 시간 (초)
    float SettlingDuration = 0.f;

    // Settling 페이즈 경과 시간 (초)
    float SettlingElapsed = 0.f;

    // Settling 시작 시점의 메시 회전
    FQuat SettlingStartQuat = FQuat::Identity;

    // Settling 목표 회전
    FQuat SettlingTargetQuat = FQuat::Identity;

    // 현재 3축 각속도 (deg/s)
    FRotator RollAngularVelocity = FRotator::ZeroRotator;

    // 보간 목표 3축 각속도 (deg/s). VelocityChangeInterval마다 새로 랜덤 결정
    FRotator TargetAngularVelocity = FRotator::ZeroRotator;

    // 다음 목표 각속도 갱신까지 남은 시간 (초)
    float VelocityChangeTimer = 0.f;

    // 이번 굴림의 목표 주사위 숫자
    int32 PendingTargetNumber = 1;
};
