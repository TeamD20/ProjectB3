본 파일은 AntiGravity 사용에 제약사항과 숙지사항을 정리해 담은 ReadMe 파일입니다.
앞으로 작업할때는 항상 이 규칙에 의해 저의 코딩을 서포트 해주시기 바랍니다.
해당 파일은 UI폴더 내부 파일과 그에 관련되어 있는 소스등에만 적용되는 ReadMe 파일입니다.
이점을 숙지하시어 다른 팀원들과의 소통에 혼동이 없도록 해주시기 바랍니다.

또한 이곳은 저의 요청없이 함부러 내용을 수정하지 말아주세요.
추가 요청을 하더라도 주변 내용을 건들지 말고 충돌의 여지가 있는 내용은 언급을 먼저 진행해주세요.

[ 제약 사항 ]
1. 코드를 함부러 먼저 수정하지 말아주세요. 제가 이해하지 못한 상태로 수정되어 곤란했던 적이 여러번 있었습니다. 먼저 제안을 해주시기 바랍니다.
2. 코드에 대한 이해를 목적으로 단순 제작-완성이 아닌 충분한 학습 가이드를 진행해주세요 
예) 이건 이렇게 되서 이런 원리로 이렇게 되는 것입니다. (육하원칙으로 왜 이렇게 되는건지 자세히 풀이해주시면 더 좋습니다.)
3. 2번 제약사항은 제가 무시하고 대답을 요구했을때 그 때 단 1회만을 진행해주세요. 그 이후는 다시 2번의 제약사향을 적용합니다.
(하지만 만일 제가 횟수로 무시해달라고 요구했을때는 그 횟수 또는 지정한 범위까지 예외적으로 적용이 가능합니다.)


[ 숙지 사항 ]
PR 메시지 작성 예시

제목: 
feat: UI 시스템 & ViewModel 레이어 추가

본문:
## 개요

- UI 위젯의 Push/Pop 스택 관리와 ViewModel 기반 데이터 바인딩 시스템을 구축
- 게임 로직(Model)과 UI(View)를 ViewModel로 분리하여 양쪽을 독립적으로 개발할 수 있는 구조를 제공

## 주요 변경사항

### UI 기반 구조
- **PBWidgetBase** — 모든 UI 위젯의 부모 클래스. InputMode, 마우스 커서 표시 여부,
ViewModel Visibility 바인딩 헬퍼 제공
- **PBUIManagerSubsystem** — LocalPlayer 기반 UI 스택 매니저. Push/Pop 시
InputMode·커서를 스택 최상위 기준으로 자동 적용
- **PBUIBlueprintLibrary** — UI/ViewModel 접근용 Blueprint 헬퍼 함수 모음.
`ExpandEnumAsExecs` 기반 Exec Pin 분기 지원

### ViewModel 시스템
- **PBViewModelBase** — ViewModel 기본 클래스. Visibility
관리(Desired/Override), 델리게이트 기반 데이터 변경 알림
- **PBViewModelSubsystem** — ViewModel 중앙 레지스트리. Global/Actor-Bound 모드
지원, Actor-Bound ViewModel은 Actor 파괴 시 자동 정리
- **Example/** — TurnOrder(Global), CharacterStat(Actor-Bound) 예시
ViewModel·Widget 쌍
- **Test/** — 에디터에서 데이터 바인딩을 검증할 수 있는 테스트 액터

### 유틸리티
- **PBBlueprintTypes** — `EPBValidResult`, `EPBFoundResult` 등 Exec Pin
분기용 공용 Enum
- **PBUITags** — UI 관련 Native GameplayTag 분리 선언
(`UI.ViewModel.TurnOrder`, `UI.ViewModel.CharacterStat` 등)
- **PBGameplayTags** — UI 태그 분리에 따른 주석 갱신

## 참고
- ViewModel 상세 구조 및 사용법은 `Source/ProjectB3/UI/ViewModel/README.md` 참조
```

PR 메시지에 포함 되어야 하는 내용

1. **개요:** 변경사항에 대한 전반적인 개요 작성
2. **주요 변경사항:**
    1. 코드 변경사항: **주요 클래스, 주요 메서드**에 대한 간단 명료한 설명,
    2. BP, 에셋 변경사항: 파일 이름 및 변경된 부분 기록
3. **TroubleShooting:** 발생한 문제에 대한 해결 혹은 조치 사항

위 예시를 참고하여 일일 커밋 내용을 요청하였을때 PR 메시지를 작성해주시기 바랍니다.

AntiGravityReadMe.txt를 말하기 편하게 그냥 지침으로 명명하겠습니다. 제가 지침이라 말하면 그것은 이곳을 말한 것 입니다.


[PR메세지 작성 공간 ]

75번째 라인부터 작성해주시면 됩니다. 앞으로 추가적인 PR메세지를 작성할때는 덮어쓰기 하여 무조건 75번째 라인부터 작성 부탁드립니다.

제목: 
feat: 파티 멤버 리스트 위젯 배경 이미지 추가 및 C++ 제어 기능 구현

본문:
## 개요
- 파티 멤버 리스트 위젯에 독립적인 배경 이미지 기능을 추가하고, 이를 블루프린트가 아닌 C++ 코드 레벨에서 효과적으로 관리 및 제어할 수 있는 구조를 구축했습니다.

## 주요 변경사항

### 코드 변경사항
- **PBPartyMemberListWidget** — 블루프린트에서 배경 위젯을 매칭할 수 있는 `BackgroundImage` (`UImage`) 변수를 추가하였으며, 필수 바인딩으로 인한 크래시를 방지하기 위해 `BindWidgetOptional` 매크로를 사용했습니다.
- **PBPartyMemberListWidget** — 동적인 배경 관리를 위해 텍스처를 변경하는 `SetBackgroundImage(UTexture2D*)`와 색상 및 투명도를 조절하는 `SetBackgroundColor(FLinearColor)` 메서드를 새롭게 구현하여 기능성을 확장했습니다.
- **PBPartyMemberListWidget** — 블루프린트에서 위젯이 바인딩되지 않은 상태에서 함수 호출이 발생할 경우를 대비해 `UE_LOG` 경고 메시지를 출력하도록 하여 디버깅 직관성을 높였습니다.

### BP, 에셋 변경사항
- 파티 멤버 리스트 UI를 담당하는 블루프린트 위젯(예: `WBP_PartyMemberList`)에 `BackgroundImage`라는 이름의 Image 컴포넌트를 배치하면 C++ 클래스와 자동으로 연동되도록 베이스 작업이 완료되었습니다.

## TroubleShooting
- **발생/방지된 문제:** 백그라운드 이미지를 C++ 변수와 매칭하도록 강제(`BindWidget`)할 경우, 해당 컴포넌트를 아직 추가하지 않은 기존 블루프린트 위젯 로드 시 크래시나 에러가 발생할 우려가 있었습니다.
- **해결 조치:** `BindWidgetOptional`을 적용하여 UI 에셋 수정 여부와 관계없이 기존 에셋과의 하위 호환성을 유지시켰으며, 이후 `BackgroundImage` 포인터를 참조하는 모든 메서드에 Null 체크 로직을 포함시켜 안전성을 확보했습니다.