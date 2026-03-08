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
feat: 파티 멤버 리스트 툴팁 UI 및 직업 정보 표기 기능 추가

본문:
## 개요
- 게임 내 파티 멤버 프로필에 마우스를 올렸을 때(Hover) 캐릭터의 이름, 레벨, 직업 정보를 표시하는 툴팁 UI를 동적으로 생성하고 뷰모델 데이터를 바인딩하는 시스템을 구축했습니다.
- 캐릭터 정보를 그리는 파티 위젯과 툴팁 위젯이 같은 데이터를 바라보도록 기존의 단일 ViewModel(`PBPartyMemberViewModel`) 구조를 확장하여 데이터 관리를 최적화했습니다.

## 주요 변경사항

### 코드 변경사항
- **PBPartyMemberViewModel** — 캐릭터 직업 정보를 보관할 `FText CharacterClass` 변수와 `Get/Set` 메서드를 추가하고, 변경 시 UI를 갱신할 `OnClassChanged` 델리게이트를 선언 및 바인딩했습니다.
- **PBPartyMemberTooltipWidget** — `UPBWidgetBase`를 상속받은 툴팁 전용 위젯 기초 클래스입니다. 이름, 레벨, 직업을 위한 텍스트 블록 컴포넌트를 정의하고, `InitializeTooltip()` 메서드에서 ViewModel 데이터를 전달받아 일정한 양식(`Name : {0}`, `Lv. {0}`, ` / Class : {0}`)으로 UI에 초기화 및 갱신하도록 구현했습니다.
- **PBPartyMemberWidget** — 블루프린트에서 툴팁 위젯 생성 시 ViewModel을 전달할 수 있도록 기존에 존재하던 `MemberViewModel` 멤버 변수 속성에 `BlueprintReadWrite` 및 `EditAnywhere`를 추가하여 접근을 허용했습니다.
- **PBUITestGameMode** — 테스트 환경에서 툴팁이 제대로 동작하는지 확인하기 위해, 4인의 더미 캐릭터가 생성될 때 차례대로 "Warrior", "Ranger", "Mage", "Cleric" 직업 정보가 순환 할당되도록 임시 부여 로직을 추가했습니다.

### BP, 에셋 변경사항
- **WBP_PartyMemberTooltip** — 새롭게 제작된 툴팁 블루프린트 에셋입니다. `Size Box` 안에 `Vertical Box`를 배치하여 툴팁의 최소/최대 가로 크기를 제한하였고, C++ 클래스에서 요구하는 이름(`CharacterNameTextBlock` 등)으로 변수를 매칭하여 텍스트 UI를 배치했습니다.
- **WBP_PartyCard** — 호버 판정을 받는 최상단 Border 컴포넌트의 `On Mouse Enter` 이벤트에 툴팁 동적 생성 로직을 추가했습니다. 마우스 진입 시 `Create Widget`으로 툴팁을 인스턴스화하고 현재의 `MemberViewModel`을 삽입한 뒤, `Set Tool Tip Widget` 노드를 통해 카드의 툴팁 속성으로 지정되도록 변경했습니다.

## TroubleShooting
- **발생 문제:** 블루프린트에서 `WBP_PartyCard`의 이벤트 노드 끄트머리에 `Add To Viewport` 노드를 호출하여, 툴팁이 마우스를 따라다니지 않고 화면 좌측 상단(0,0)에 고정되어 렌더링되는 버그가 발생했습니다.
- **해결 조치:** 언리얼 엔진의 툴팁 렌더링 시스템은 `Set Tool Tip Widget`으로 대상을 연결해 주면 엔진이 알아서 커서 위치에 띄우고 지우는 생명 주기를 관리한다는 점을 확인했습니다. 따라서 강제로 화면에 그리는 `Add To Viewport` 노드를 삭제하여 마우스 포인터를 따라다니는 정상적인 툴팁 위치 표기로 수정했습니다.