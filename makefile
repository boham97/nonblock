CC = gcc
CFLAGS = -O3 -march=native -flto -fno-plt -fomit-frame-pointer
LDFLAGS = -lpthread

# 타겟 실행 파일
TARGET = queue

# 소스 파일
SRCS = queue.c queue_test.c

# 오브젝트 파일
OBJS = $(SRCS:.c=.o)

# 기본 타겟
all: $(TARGET)

# 실행 파일 생성
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# .c -> .o 컴파일
%.o: %.c queue.h
	$(CC) $(CFLAGS) -c $<

# 실행
run: $(TARGET)
	./$(TARGET)

# 벤치마크 (최적화 레벨 비교)
benchmark: clean
	@echo "=== O0 (최적화 없음) ==="
	@$(CC) -O0 -g -o $(TARGET)_O0 $(SRCS) $(LDFLAGS)
	@./$(TARGET)_O0
	@echo ""
	@echo "=== O2 ==="
	@$(CC) -O2 -g -o $(TARGET)_O2 $(SRCS) $(LDFLAGS)
	@./$(TARGET)_O2
	@echo ""
	@echo "=== O3 ==="
	@$(CC) -O3 -g -o $(TARGET)_O3 $(SRCS) $(LDFLAGS)
	@./$(TARGET)_O3
	@echo ""
	@echo "=== O3 -march=native ==="
	@$(CC) -O3 -march=native -g -o $(TARGET)_O3native $(SRCS) $(LDFLAGS)
	@./$(TARGET)_O3native

# 디버그 빌드
debug: CFLAGS = -O0 -g -Wall -Wextra -DDEBUG
debug: clean $(TARGET)

# 릴리즈 빌드 (디버그 심볼 없음)
release: CFLAGS = -O3 -march=native -Wall -Wextra -DNDEBUG
release: clean $(TARGET)

# 정리
clean:
	rm -f $(OBJS) $(TARGET) $(TARGET)_O0 $(TARGET)_O2 $(TARGET)_O3 $(TARGET)_O3native

# 재빌드
rebuild: clean all

# 헤더 파일 의존성
queue.o: queue.c queue.h
queue_test.o: queue_test.c queue.h

.PHONY: all run benchmark debug release clean rebuild
