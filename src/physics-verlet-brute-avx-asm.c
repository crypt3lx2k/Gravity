#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <immintrin.h>

#include "align_malloc.h"
#include "physics.h"

extern value grav;
extern value soft;
extern value half;

extern value * a0x;
extern value * a0y;
extern value * a1x;
extern value * a1y;

value grav = GRAVITATIONAL_CONSTANT;
value soft = SOFTENING;
value half = value_literal(0.5);

value * a0x = NULL;
value * a0y = NULL;

value * a1x = NULL;
value * a1y = NULL;

static size_t allocated = 0;

void physics_free (void) {
  align_free(a1y);
  align_free(a1x);
  align_free(a0y);
  align_free(a0x);

  a0x = NULL;
  a0y = NULL;
  a1x = NULL;
  a1y = NULL;
}

void physics_init (size_t n) {
  allocated = n;

  a0x = align_malloc(ALIGN_BOUNDARY, n*sizeof(value) + ALLOC_PADDING);
  a0y = align_malloc(ALIGN_BOUNDARY, n*sizeof(value) + ALLOC_PADDING);
  a1x = align_malloc(ALIGN_BOUNDARY, n*sizeof(value) + ALLOC_PADDING);
  a1y = align_malloc(ALIGN_BOUNDARY, n*sizeof(value) + ALLOC_PADDING);

  if (a0x == NULL || a0y == NULL || a1x == NULL || a1y == NULL) {
    perror(__func__);
    exit(EXIT_FAILURE);
  }

  physics_reset();
}

void physics_reset (void) {
  memset(a0x, 0, allocated*sizeof(value));
  memset(a0y, 0, allocated*sizeof(value));
  memset(a1x, 0, allocated*sizeof(value));
  memset(a1y, 0, allocated*sizeof(value));
}