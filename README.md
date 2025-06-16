# Renzzle
<img width="1440" alt="Image" src="https://github.com/user-attachments/assets/e7054f08-4d15-46c5-96ff-8a3fad3a3c09" style="width: 100%;"/>

## 🎯 Project Overview

Renzzle은 오목 초보자부터 전문가까지 누구나 즐기며 실력을 향상시킬 수 있는 **AI 기반 오목 퍼즐 학습 플랫폼**입니다.

* **트레이닝 퍼즐:** 정교한 AI를 상대로 단계별 퍼즐을 풀며 체계적인 학습이 가능합니다.
* **커뮤니티 퍼즐:** 사용자가 직접 퍼즐을 만들고 공유하며, 다른 유저의 퍼즐에 도전할 수 있습니다.
* **AI 문제 검증:** 유저가 만든 모든 퍼즐은 AI가 풀이 가능성을 검증하여 콘텐츠의 질을 보장합니다.
* **랭킹 시스템:** 랭킹전을 통해 다른 사용자들과 실력을 겨루며 학습 동기를 부여합니다.

## 📖 Repository Overview
이 Repository는 C++로 개발된 오목(Renju) AI 엔진입니다. <br>
핵심 원리는 Alpha-Beta Pruning 탐색 알고리즘을 사용하며, 여기에 다양한 최적화 기법을 적용하여 높은 기력과 빠른 수읽기 속도를 목표로 합니다. <br>
이 AI는 사용자의 수에 대한 올바른 대응을 찾고, 사용자가 창작한 퍼즐들의 풀이 가능성을 검증하는 역할을 합니다.

## 🛠️ Core Technologies
- **Alpha-Beta Pruning**
  - 기본 탐색 알고리즘으로, 불필요한 노드 탐색을 줄여 효율을 높입니다. search.h 파일의 abp 함수에서 스택 기반의 비재귀 방식으로 구현되어 있습니다.

- **Iterative Deepening Search**
  - 제한 시간 내에 최적의 탐색 깊이를 동적으로 결정합니다. search.h의 ids 함수는 낮은 깊이부터 탐색을 시작하여 점차 깊이를 늘려나갑니다.

- **Transposition Table / Zobrist Hashing**
  - 이미 탐색했던 수순의 결과를 저장하고 재사용하여 중복 연산을 제거합니다. tree/tree.h와 tree/tree_manager.h에서 이를 관리하며, game/zobrist.h를 통해 각 보드 상태의 고유 키를 빠르게 생성합니다.

- **Principal Variation Search**
  - 최선의 수순(PV)일 것이라 예상되는 노드는 전체 창(Full Window)으로, 나머지 노드는 최소 창(Null Window)으로 탐색하여 기존 알파-베타 탐색보다 효율을 극대화합니다. search.h의 SearchMode 열거형과 abp 함수 로직에서 이 기법이 사용된 것을 확인할 수 있습니다.

- **Pattern-Based Evaluation**
  - 현재 국면의 유불리를 판단하기 위해 '열린 3', '닫힌 4' 등 다양한 돌의 형태(패턴)에 점수를 매겨 평가 함수를 구성합니다. evaluate/evaluator.h 및 game/cell.h에 패턴과 점수 체계가 정의되어 있습니다.

- **Heuristic Move Generation & Ordering**
  - Evaluator 클래스는 getCandidates 함수 등을 통해 유망한 후보 수를 효과적으로 생성합니다. 또한 Search 클래스의 sortChildNodes 함수는 탐색 순서를 최적화하여 Alpha-Beta Pruning의 효율을 크게 향상시킵니다.

- **Victory by Continuous Four Search**
  - 필승 수순인 VCF(연속적인 4 공격)를 찾는 별도의 전문 탐색 루틴(search_win.h)을 구현하여, 필승 국면에서 결정적인 수를 빠르게 찾아냅니다.

- **Renju Rule**
  - 흑의 3-3, 4-4, 장목(Overline) 등 공식 렌주 룰에 따른 금수 판정 로직이 board.h의 isForbidden 함수에 정교하게 구현되어 있습니다.
