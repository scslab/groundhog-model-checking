#pragma once

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

typedef uint8_t choice_t;
typedef uint64_t hash_t;

choice_t choose(choice_t n, hash_t hash);
void report_error();
void check_exit_normal();
extern void check_main();
