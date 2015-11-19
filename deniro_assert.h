#ifndef DENIRO_ASSERT_H
#define DENIRO_ASSERT_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PRINT_FAIL printf("************ FAIL IN FILE %s LINE %d\n", \
                          __FILE__, __LINE__);

#define den_assert(test) if (!(test)) {  PRINT_FAIL }

#define den_assert_str_eq(str1, str2) \
  if (!(str1)) { printf("Expected value is NULL\n"); PRINT_FAIL } \
  if (!(strcmp(str1, str2) == 0)) { \
    printf("*********** STRINGS DON'T MATCH expected: \"%s\" \t got: \"%s\" \t", \
           str2, str1); PRINT_FAIL }

#endif
