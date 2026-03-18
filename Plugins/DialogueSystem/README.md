# DialogueSystem 플러그인

대화 데이터를 그래프 기반으로 편집하고, 런타임에서 노드 단위로 진행시키는 플러그인.
`Runtime`은 대화 실행 로직, `Editor`는 `DialogueData` 전용 그래프 에디터를 제공한다.

---

## 구조

```
Plugins/DialogueSystem/
├── DialogueSystem.uplugin
└── Source/
    ├── DialogueSystemRuntime/
    │   ├── Public/
    │   │   ├── DialogueData.h                 대화 데이터 에셋 (노드 배열, StartNodeID)
    │   │   ├── DialogueManagerComponent.h     대화 시작/진행/종료 관리 컴포넌트
    │   │   ├── DialogueInstance.h             NodeID -> Node 조회 캐시
    │   │   ├── DialogueNode.h                 대화 노드 + Feature 컨테이너
    │   │   ├── DialogueSystemTypes.h          Context/Participants/ChangeMessage 타입
    │   │   └── DialogueFeatures/
    │   │       ├── DNodeFeature_Text.h        텍스트 출력 기능
    │   │       ├── DNodeFeature_ChoiceBranch.h 선택지 분기
    │   │       └── DNodeFeature_DefaultLinkBranch.h 기본 단일 분기
    │   └── Private/
    │
    └── DialogueSystemEditor/
        ├── Public/DialogueGraphEditor/        DialogueData 커스텀 에디터 앱/스키마
        ├── Public/DialogueGraphNodes/         Start/Content/End 노드 에디터 표현
        ├── Public/DialogueGraphFactories/     그래프 노드/핀 시각화 팩토리
        └── Private/
```

---

## 핵심 개념

### 1) DialogueData + DialogueNode

- `UDialogueData`는 대화 전체를 보관하는 DataAsset이다.
- `StartNodeID`와 `DialogueNodes`를 기반으로 런타임 탐색이 시작된다.
- 각 `UDialogueNode`는 `ParticipantTag`와 `NodeFeatures`를 가진다.

### 2) NodeFeature 조합

- 노드 동작은 `UDNodeFeature` 파생 클래스로 분리된다.
- 각 Feature는 노드 진입 시(`OnStartDialogueNode`)와 노드 이탈 시(`OnEndDialogueNode`) 실행될 동작을 정의할 수 있다.
- 분기 구현은 기본 링크형(`DefaultLinkBranch`)과 선택지형(`ChoiceBranch`)을 제공하며, 이를 통해 그래프가 구성된다.
- 커스텀 Feature를 추가하면 조건 검사, 연출 트리거, 상태 기록 등 프로젝트별 로직을 노드 단위로 확장할 수 있다.

### 3) DialogueManagerComponent 흐름

1. `StartDialogue(DialogueData, Context)`
2. `DialogueInstance` 생성 및 `StartNodeID`로 현재 노드 설정
3. 노드 전환 시 Feature `OnStartDialogueNode/OnEndDialogueNode` 호출
4. `ProgressDialogue(OptionIndex)`로 Branch가 다음 노드 ID 결정
5. 시작/변경/종료 시 Delegate 브로드캐스트

---

## 빠른 사용

### 1. DialogueData 생성

- 콘텐츠 브라우저에서 `Dialogue Data` 에셋 생성
- 그래프 에디터에서 Start/Content/End 노드 연결
- Content 노드에 Text/Branch Feature 설정

### 2. 매니저 컴포넌트 부착

- 일반적으로 `PlayerController`에 `UDialogueManagerComponent`(또는 파생 클래스) 부착

### 3. 컨텍스트 구성 후 시작

```cpp
FDialogueSystemContext Context;
Context.ContextObject = this;
Context.InstigatorController = PC;
Context.InstigatorActor = PC ? PC->GetPawn() : nullptr;
Context.TargetActor = TargetNPC;

DialogueManager->StartDialogue(DialogueData, Context);
```

### 4. UI/입력에서 진행

```cpp
// 기본 진행(단일 분기)
DialogueManager->ProgressDialogue();

// 선택지 분기
DialogueManager->ProgressDialogue(SelectedOptionIndex);
```

---

## 확장 포인트

- `PreStartDialogue`: 시작 직전 프로젝트별 사전 준비 로직
- `OnDialogueStart / OnDialogueChanged / OnDialogueEnd`: UI 표시, 상태 동기화, 정리
- 커스텀 `UDNodeFeature`: 조건 검사, 연출 트리거, 게임플레이 이벤트 연동

---

## ProjectB3 활용 예시

`ProjectB3`는 플러그인 기본 컴포넌트를 `UPBDialogueManagerComponent`로 확장해,
대화 시작 시 ViewModel 생성, 대화 UI Push/Pop, 화자 정보 반영, 주사위 판정 결과 처리까지 프로젝트 규칙에 맞게 연결해 사용한다.

핵심은 플러그인 Runtime을 그대로 두고, 매니저 override + Feature 확장으로 게임별 정책만 얹는 방식이다.
