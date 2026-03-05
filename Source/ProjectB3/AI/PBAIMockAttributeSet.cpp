// PBAIMockAttributeSet.cpp
#include "PBAIMockAttributeSet.h"
#include "Net/UnrealNetwork.h" // GetLifetimeReplicatedProps 등 멀티플레이 확장을 위해 대기

UPBAIMockAttributeSet::UPBAIMockAttributeSet() {
  // 초기화
  InitHealth(100.0f);
  InitAction(1.0f);
  InitBonusAction(1.0f);
  InitMovement(0.0f);
  InitMaxMovement(900.0f);
}
