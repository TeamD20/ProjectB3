# ViewModel 시스템

게임 로직(Model)과 UI(View)를 분리하는 Presentation Model 레이어.
ViewModel이 표시용 데이터와 변환 로직을 보유하고, 델리게이트로 Widget에 변경을 알린다.

---

## 구조

```
Source/ProjectB3/UI/
├── PBWidgetBase.h                    위젯 기본 클래스 (InputMode, 커서, Visibility 바인딩)
├── PBUIManagerSubsystem.h/.cpp       Push/Pop 스택 관리, InputMode 자동 적용
├── PBUIBlueprintLibrary.h/.cpp       BP/C++ 헬퍼 함수
│
└── ViewModel/
    ├── PBViewModelBase.h/.cpp        ViewModel 기본 클래스
    ├── PBViewModelSubsystem.h/.cpp   ViewModel 중앙 레지스트리
    │
    ├── Example/                      학습용 예시
    │   ├── PBExampleTurnOrderViewModel        (Global)
    │   ├── PBExampleCharacterStatViewModel    (Actor-Bound)
    │   ├── PBExampleTurnOrderWidget           (Slate 위젯)
    │   └── PBExampleCharacterStatWidget       (Slate 위젯)
    │
    └── Test/                         데이터 바인딩 테스트 액터
        ├── PBTestTurnOrderActor
        └── PBTestCharacterStatActor
```

---

## 핵심 개념

### Global vs Actor-Bound

| | Global | Actor-Bound |
|---|---|---|
| **용도** | 플레이어 전역 상태 (턴 순서, 설정) | 특정 Actor에 종속 (체력바, NPC 정보) |
| **생성** | `GetOrCreateGlobalViewModel` | `GetOrCreateActorViewModel(Actor, ...)` |
| **인스턴스** | 클래스당 1개 | (Actor, Class) 쌍당 1개 |
| **정리** | 수동 `Unregister` 또는 Subsystem 종료 | Actor 파괴 시 자동 (`OnDestroyed`) |

### 데이터 흐름

```
게임 로직 (Component, Ability 등)
    │
    ▼  Setter 호출 (SetHP, SetTurnOrder 등)
ViewModel (데이터 보유 + Presentation Logic)
    │
    ▼  델리게이트 Broadcast (OnHPChanged, OnTurnOrderChanged 등)
Widget (UI 갱신)
```

게임 로직은 ViewModel의 Setter만 호출하고, Widget은 ViewModel의 Getter와 델리게이트만 사용한다.
양쪽 모두 서로를 직접 참조하지 않는다.

### GetOrCreate 패턴

누가 먼저 호출하든 동일한 인스턴스를 반환한다.

- 게임 로직이 먼저 생성 → Widget이 나중에 접근 → 이미 데이터가 채워진 ViewModel 획득
- Widget이 먼저 생성 → 게임 로직이 나중에 데이터 주입 → 델리게이트로 자동 갱신

이 패턴 덕분에 **UI 개발과 게임 로직 개발을 병렬로 진행**할 수 있다.

---

## 빠른 시작

### 1. ViewModel 서브클래스 작성

```cpp
// MyViewModel.h
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoldChanged, int32, NewGold);

UCLASS(BlueprintType)
class UMyViewModel : public UPBViewModelBase
{
    GENERATED_BODY()

public:
    // Setter (외부에서 호출)
    UFUNCTION(BlueprintCallable)
    void SetGold(int32 InGold);

    // Getter (Presentation Logic)
    UFUNCTION(BlueprintPure)
    FText GetFormattedGold() const;

    // 이벤트
    UPROPERTY(BlueprintAssignable)
    FOnGoldChanged OnGoldChanged;

protected:
    UPROPERTY(BlueprintReadOnly)
    int32 Gold = 0;
};

// MyViewModel.cpp
void UMyViewModel::SetGold(int32 InGold)
{
    if (Gold != InGold)
    {
        Gold = InGold;
        OnGoldChanged.Broadcast(Gold);
    }
}

FText UMyViewModel::GetFormattedGold() const
{
    // Presentation Logic: "1,234 Gold"
    return FText::FromString(FString::Printf(TEXT("%s Gold"),
        *FText::AsNumber(Gold).ToString()));
}
```

### 2. Widget에서 바인딩

```cpp
// MyWidget.cpp
void UMyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // UPBUIBlueprintLibrary 헬퍼 활용
    CachedVM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UMyViewModel>(GetOwningLocalPlayer());

    if (CachedVM)
    {
        CachedVM->OnGoldChanged.AddDynamic(this, &ThisClass::HandleGoldChanged);
        BindVisibilityToViewModel(CachedVM);  // Visibility 자동 연동
    }
}

void UMyWidget::NativeDestruct()
{
    if (CachedVM)
    {
        CachedVM->OnGoldChanged.RemoveDynamic(this, &ThisClass::HandleGoldChanged);
        UnbindVisibilityFromViewModel(CachedVM);
    }
    Super::NativeDestruct();
}
```

### 3. 게임 로직에서 데이터 주입

`UPBUIBlueprintLibrary`는 `APlayerController*` 혹은 `ULocalPlayer*` 로부터 ViewModel을 획득할 수 있는 헬퍼 함수를 제공한다

```cpp
// C++ 템플릿 헬퍼 (Cast 불필요, static_assert로 타입 안전)
APlayerController* PC = GetWorld()->GetFirstPlayerController();
UMyViewModel* VM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UMyViewModel>(PC);

VM->SetGold(1234);  // → OnGoldChanged → Widget 자동 갱신
```

```cpp
// TSubclassOf 기반 (BP에서도 사용 가능한 non-template 버전)
UMyViewModel* VM = Cast<UMyViewModel>(UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel(PC, UMyViewModel::StaticClass()));
```

```cpp
// LocalPlayer 기반 template (Widget, Subsystem 등)
UMyViewModel* VM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UMyViewModel>(LocalPlayer);
```

### 4. Actor-Bound 패턴

```cpp
// C++ 템플릿 헬퍼
UMyEnemyVM* VM = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UMyEnemyVM>(PC, EnemyActor);

VM->SetHP(CurrentHP, MaxHP);
// EnemyActor가 파괴되면 ViewModel도 자동 정리
```

---

## Visibility 시스템

ViewModel에 내장된 2단계 가시성 제어:

| 메서드 | 역할 |
|---|---|
| `SetDesiredVisibility(bool)` | 기본 가시성 설정 |
| `SetVisibilityOverride(bool)` | 가시성 강제 오버라이드 (원래 값 보존) |
| `ClearVisibilityOverride()` | 오버라이드 해제 → 기본값 복귀 |
| `IsVisible()` | 유효 가시성 반환 |

유효 가시성 = `Override 활성 ? Override 값 : 기본값`

Widget은 `BindVisibilityToViewModel()`으로 이 가시성에 자동 연동할 수 있다.
PBWidgetBase의 `VMVisibleState`/`VMHiddenState`로 Slate Visibility 매핑을 커스터마이즈 가능.

### Tag 기반 일괄 제어

```cpp
// "UI.ViewModel.HUD" 태그를 가진 모든 ViewModel 숨기기
VMS->SetVisibilityByTag(HUDTag, false);

// 복원
VMS->RestoreVisibilityByTag(HUDTag, true);
```

---

## 장점

- **병렬 개발**: GetOrCreate 패턴으로 UI/게임로직 독립 개발. 더미 데이터로 UI 먼저 작업 가능.
- **관심사 분리**: 게임 로직은 Setter만, Widget은 Getter/델리게이트만 사용. 양쪽 변경이 서로에 영향 없음.
- **Presentation Logic 집중**: 표시 포맷팅, 계산, 상태 판정(`IsBloodied`, `GetModifier`)이 ViewModel에 집중되어 Widget이 단순해짐.
- **자동 생명주기**: Actor-Bound ViewModel은 Actor 파괴 시 자동 정리. 수동 cleanup 불필요.
- **BP/C++ 양쪽 지원**: 모든 API가 Blueprint와 C++ 모두에서 사용 가능.

## 한계 및 고려사항

- **수동 델리게이트 바인딩**: UE5 MVVM 플러그인과 달리 자동 바인딩 엔진 없음. 프로퍼티별 델리게이트를 수동 선언/구독해야 함.
- **Setter 보일러플레이트**: 프로퍼티마다 `if (Old != New) { Set; Broadcast; }` 패턴 반복. 매크로 도입 시 BP 접근 불가하므로 직접 작성.
- **단일 위젯 바인딩 제한**: `BindVisibilityToViewModel`이 마지막 바인딩만 유효 (1:1). 복수 ViewModel → 1 Widget 시나리오는 수동 처리 필요.
- **리스트 데이터**: 배열 프로퍼티 변경 시 전체 리빌드(`RebuildTurnList`). 개별 항목 추가/삭제 알림은 미지원.

## 향후 고도화 방향

- **Setter 보일러플레이트 매크로**: `if (Old != New) { Set; Broadcast; }` 패턴을 매크로로 축소. BP 접근이 필요 없는 C++ 전용 프로퍼티에 적용 가능.
- **지연 업데이트 (Deferred Update)**: 한 프레임 내 여러 Setter 호출 시 즉시 Broadcast하지 않고, 다음 프레임에 1회만 알림. `MarkDirty()` + Tick/Timer 기반.
- **List/Collection 알림**: 항목 추가/삭제/이동에 대한 개별 이벤트
- **UE5 MVVM 전환**: `UPBViewModelBase`를 `UMVVMViewModelBase`로 교체 가능 (Subsystem 레이어 유지, 서브클래스만 FieldNotify 전환)

---

## 테스트 방법

1. `PBTestTurnOrderActor`를 레벨에 배치 → Play → 상단에 턴 오더 HUD 표시
2. 디테일 패널에서 `TestAdvanceTurn` → 턴 하이라이트 이동 확인
3. `PBTestCharacterStatActor`를 레벨에 배치 → Play
4. `TestOpenStatSheet` → 캐릭터 시트 팝업 (중앙)
5. `TestTakeDamage` → HP바 감소 + Bloodied 색상 전환 확인
6. `TestCloseStatSheet` → 팝업 닫기
7. `TestDestroyActor` → Actor 파괴 후 ViewModel 자동 정리 확인
