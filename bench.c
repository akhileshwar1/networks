#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

static inline uint64_t now_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

typedef struct {
  size_t size;
} MallocArgs;

void malloc_bench(void *arg) {
  MallocArgs *args = (MallocArgs *)arg;
  uint64_t start = now_ns();
  void *p = malloc(args->size);
  uint64_t end = now_ns();
  uint64_t delta = end - start;  // in nanoseconds
  free(p);
  printf("latency: %" PRIu64 " ns\n", delta);
}

void memcpy_bench(void *arg) {
  size_t size = 1024 * 1024;
  void *src = malloc(size);
  void *dst = malloc(size);

  if (!src || !dst) {
    printf("malloc failed \n");
  }

  memset(src, 1, size);
  uint64_t start = now_ns();
  memcpy(dst, src, size);
  uint64_t end = now_ns();
  uint64_t delta = end - start;  // in nanoseconds
  printf("latency: %" PRIu64 " ns\n", delta);
}

void hey(void *arg) {
  uint64_t start = now_ns();
  /* printf("hey, %s\n", (char *)arg); */
  uint64_t end = now_ns();
  uint64_t delta = end - start;  // in nanoseconds
  printf("latency: %" PRIu64 " ns\n", delta);
}

typedef void (*bench_fn)(void *);

typedef struct {
  const char *name;
  bench_fn func;
} BenchmarkEntry;

BenchmarkEntry benchmarks[] = {
  {"memcpy", memcpy_bench},
  {"hey", hey},
  {"malloc", malloc_bench}
};

bench_fn find_benchmark(const char *name) {
  for (int i = 0; benchmarks[i].name != NULL; ++i){
    if (strcmp(name, benchmarks[i].name) == 0)
      return benchmarks[i].func;
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <func_name> <arg> \n", argv[0]);
    return 1;
  }

  bench_fn fn = find_benchmark(argv[1]);
  if (!fn) {
    fprintf(stderr, "Function %s not found", argv[1]);
  }

  fn(argv[2]);
  return 0;
}
