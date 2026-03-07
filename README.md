# Non-Blocking Concurrent Queue (Michael-Scott Queue)

C언어로 구현한 Lock-Free MPMC(Multi-Producer Multi-Consumer) 큐.

**참고 논문**: M. Michael, M. Scott — *Simple, Fast, and Practical Non-Blocking and Blocking Concurrent Queue Algorithms*, PODC 1996
https://www.cs.rochester.edu/u/scott/papers/1996_PODC_queues.pdf

---

## 알고리즘 개요

Michael-Scott Queue는 `CAS(Compare-And-Swap)` 하나만으로 락 없이 동시 enqueue/dequeue를 처리하는 링크드 리스트 기반 큐다.

### 핵심 아이디어: Tagged Pointer로 ABA 문제 해결

x86-64는 실제로 가상 주소 48비트만 사용하므로, 상위 16비트를 버전 태그로 활용한다.

```
 63        48 47                  0
 ┌──────────┬──────────────────────┐
 │  tag(16) │    pointer(48)       │
 └──────────┴──────────────────────┘
```

CAS가 성공할 때마다 태그를 1씩 증가시켜, 같은 주소가 재사용되더라도 태그가 달라 ABA를 탐지한다.

### Enqueue

```
1. tail과 tail->next를 읽는다
2. tail->next가 NULL이면
     CAS(tail->next, NULL, new_node)  → 성공 시 tail을 전진
3. tail->next가 NULL이 아니면 (tail이 뒤처진 경우)
     CAS(tail, old_tail, next)        → tail 최신화 후 재시도
```

### Dequeue

```
1. head, tail, head->next를 읽는다
2. head == tail이고 head->next == NULL → 큐 비어 있음 (NULL 반환)
3. head == tail이고 head->next != NULL → tail 뒤처짐 → tail 전진
4. 그 외: 값을 미리 읽고 CAS(head, old_head, next) → 성공 시 old dummy 반환
```

> dummy 노드 기법을 사용하므로 head는 항상 이미 소비된 노드를 가리키고,
> 실제 값은 `head->next`에 있다.

---

## 구현 세부사항

### CAS 재시도 전략

CAS 실패 시 `sched_yield()`로 CPU를 양보한다.

`_mm_pause()` 기반 스핀웨이트, 지수 백오프 등 여러 전략을 벤치마크로 비교했으나,
고경합(스레드 수 많음) 환경에서는 `sched_yield()`가 가장 좋은 처리량을 보였다.

- `_mm_pause()` 단독: syscall 없이 빠르지만 모든 스레드가 동시에 같은 캐시 라인을 읽어 MESI invalidation이 폭주함
- `sched_yield()`: syscall 비용이 있지만 스레드를 실제로 재워 경쟁자 수를 줄이고 캐시 경합을 완화함

> 저경합 환경에서는 `_mm_pause()`가 유리할 수 있으므로 워크로드에 맞게 선택한다.

### 캐시 라인 패딩

`head`와 `tail`은 서로 다른 캐시 라인(64B)에 위치시켜 False Sharing을 방지한다.

```c
typedef struct Queue {
    uint64_t head;
    char padding[64 - sizeof(uint64_t)];  // head를 캐시 라인 분리
    uint64_t tail;
    char padding2[64 - sizeof(uint64_t)];
} Queue;
```

### 노드 풀 (Pool Allocator)

벤치마크 측정 구간에서 `malloc`/`free` 오버헤드를 제거하기 위해,
각 Producer 스레드가 시작 전에 `OPS`개의 노드를 일괄 할당해 둔다.

```c
// main에서 배리어 전에 미리 할당
pools[i] = malloc(OPS * sizeof(Node));

// producer에서 풀 노드를 직접 enqueue
enqueue_node(&q, &pool[i]);
```

### 동시 출발 보장 (pthread_barrier)

스레드 생성 순서에 의한 유불리를 없애기 위해 배리어로 모든 스레드가 동시에 작업을 시작하게 한다.
main도 배리어에 참여해 모든 스레드가 준비된 직후 시작 시각을 기록한다.

```
[pthread_create × 2×THREADS]
        ↓
main: clock_gettime(&start) → barrier_wait
        ↓  ← 모든 스레드 동시 해제
[실제 작업]
        ↓
[pthread_join 완료] → clock_gettime(&end)
```

---

## 파일 구조

```
.
├── queue.h          # Node/Queue 타입, Tagged Pointer 매크로/인라인 함수
├── queue.c          # initQueue / enqueue / enqueue_node / dequeue 구현
├── queue_test.c     # 멀티스레드 벤치마크 (Producer/Consumer)
└── Makefile
```

---

## 빌드 및 실행

```bash
# 기본 빌드 (O3 + march=native + lto)
make

# 실행
make run

# 최적화 레벨별 비교 벤치마크
make benchmark

# 디버그 빌드
make debug

# 정리
make clean
```

---

## 벤치마크 설정

| 항목 | 값 |
|------|----|
| `THREADS` | 12 (Producer 12 + Consumer 12) |
| `OPS` | 1,000,000 ops/thread |
| 총 enqueue | 12,000,000 |
| 컴파일 플래그 | `-O3 -march=native -flto` |

출력 예시:
```
th 0 append: 1000000
th 1 append: 1000000
...
===== Consumed Per Thread =====
Consumer  0: 1043821
Consumer  1:  998732
...
Total consumed: 12000000
Elapsed Time : 1.83 sec
Throughput : 6557377 ops/sec
===============================
```
