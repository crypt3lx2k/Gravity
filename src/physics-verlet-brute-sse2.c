#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <emmintrin.h>

#include "align_malloc.h"
#include "physics.h"

static const value G = GRAVITATIONAL_CONSTANT;

static vector * a0 = NULL;
static vector * a1 = NULL;

static size_t allocated = 0;

static inline void physics_swap (void) {
  vector * t = a0;
  a0 = a1;
  a1 = t;
}

void physics_advance (particles * p, value dt) {
  size_t i, j;
  size_t n = p->n;

  /* g = G, repeat.  */
  const __m128 g = _mm_set1_ps(G);
  /* s = epsilon^2, repeat. */
  const __m128 s = _mm_set1_ps(SOFTENING*SOFTENING);

#pragma omp parallel private(i, j)
  {
#pragma omp for
    for (i = 0; i < n; i++) {
      p->x[i][0] +=
	(p->v[i][0] + value_literal(0.5)*a0[i][0]*dt)*dt;
      p->x[i][1] +=
	(p->v[i][1] + value_literal(0.5)*a0[i][1]*dt)*dt;

      a1[i][0] = value_literal(0.0);
      a1[i][1] = value_literal(0.0);
    }

#pragma omp for
    for (i = 0; i < n; i += 2) {
      __m128 xi, ai;

      /* xi = p->x[i][0], p->x[i][1], p->x[i+1][0], ... */
      xi = _mm_load_ps(&p->x[i][0]);
      /* ai = a1[i][0], a1[i][1], a1[i+1][0], ... */
      ai = _mm_load_ps(&a1[i][0]);

      for (j = 0; j < n; j++) {
	__m128 xj;
	__m128 d;
	__m128 d2, b2;
	__m128 mj;
	__m128 r, r3s2, a;

	/* xj = p->x[j][0], p->x[j][1], ... */
	xj = _mm_castpd_ps (
	       _mm_load1_pd((double *) &p->x[j][0])
	);

	/* mj = p->m[j], ... */
	mj = _mm_load1_ps(&p->m[j]);

	/* d = dx_ji, dy_ji, dx_j(i+1) dy_j(i+1), ... */
	d = _mm_sub_ps(xj, xi);

	/* r = 1/sqrt(r_ji^2+epsilon^2), ..., 1/sqrt(r_j(i+1)^2+epsilon^2), ... */
	d2 = _mm_mul_ps(d, d);
	b2 = _mm_shuffle_ps(d2, d2, 0b10110001);
	r = _mm_add_ps(d2, b2);
	r = _mm_add_ps(r, s);
	r = _mm_rsqrt_ps(r);

	/* r3s2 = 1/(r^2 + epsilon^2)^3/2 */
	r3s2 = _mm_mul_ps(r, r);
	r3s2 = _mm_mul_ps(r3s2, r);

	/* We omit the m_i factors as they disappear in
	   a_i = F_i/m_i anyway. */
	/* a = m_j*d*G/(r^2 + epsilon^2)^(3/2) */
	a = _mm_mul_ps(g, r3s2);
	a = _mm_mul_ps(a, d);
	a = _mm_mul_ps(a, mj);

	/* ai += a */
	ai = _mm_add_ps(ai, a);
      }

      _mm_store_ps(&a1[i][0], ai);
    }

#pragma omp for
    for (i = 0; i < n; i++) {
      p->v[i][0] += value_literal(0.5)*(a0[i][0]+a1[i][0])*dt;
      p->v[i][1] += value_literal(0.5)*(a0[i][1]+a1[i][1])*dt;
    }
  } /* #pragma omp parallel */

  physics_swap();
}

void physics_free (void) {
  align_free(a0);
  align_free(a1);

  a0 = NULL;
  a1 = NULL;
}

void physics_init (size_t n) {
  allocated = n;

  a0 = align_malloc(32, n*sizeof(vector));
  a1 = align_malloc(32, n*sizeof(vector));

  if (a0 == NULL || a1 == NULL) {
    perror(__func__);
    exit(EXIT_FAILURE);
  }

  physics_reset();
}

void physics_reset (void) {
  memset(a0, 0, allocated*sizeof(vector));
  memset(a1, 0, allocated*sizeof(vector));
}