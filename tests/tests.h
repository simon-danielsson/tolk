#ifndef TESTS_H
#define TESTS_H

#include "../src/main.h"

extern uint _tests_failures, _tests_total;

#define test_assert(func, cond)                                                  do {                                                                             if (!(cond)) {                                                                   fprintf(stderr, "\033[4mFailure -> %-24s%s\033[0m\n", (func), #cond);          _tests_failures += 1;                                                        } else {                                                                         fprintf(stderr, "Success -> %-24s%s\n", (func), #cond);                      }                                                                            } while (0)

#define TEST_DIV                                                                 do {                                                                             uint j;                                                                        for (j = 0; j < 3; j++) {                                                        fprintf(stderr, "┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄");                                        }                                                                              fprintf(stderr, "\n");                                                       } while (0)

#endif

