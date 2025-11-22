#ifndef SUM_UTILS_H
#define SUM_UTILS_H

struct SumArgs {
  int *array;
  int begin;
  int end;
};

int Sum(const struct SumArgs *args);

#endif
