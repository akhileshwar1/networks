#include <stdio.h>
#include <string.h>

void hello(void *arg) {
  printf("hello, %s\n", (char *)arg);
}

void hey(void *arg) {
  printf("hey, %s\n", (char *)arg);
}

typedef void (*bench_fn)(void *);

typedef struct {
  const char * name;
  bench_fn func;
} BenchmarkEntry;

BenchmarkEntry benchmarks[] = {
  {"hello", hello},
  {"hey", hey},
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
