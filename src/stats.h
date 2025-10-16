/*
 * Copyright © 2025, Niklas Haas
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CHECKASM_STATS_H
#define CHECKASM_STATS_H

#include <assert.h>
#include <math.h>
#include <stdint.h>

#include "internal.h"

static inline double rv_stddev(const RandomVar rv)
{
    return rv.var > 0.0 ? sqrt(rv.var) : 0.0;
}

static inline RandomVar rv_const(double x)
{
    return (RandomVar){ x, 0.0 };
}

/* Assumes independent random variables */
RandomVar rv_scale(RandomVar a, double s);
RandomVar rv_add(RandomVar a, RandomVar b);
RandomVar rv_sub(RandomVar a, RandomVar b);
RandomVar rv_mul(RandomVar a, RandomVar b);
RandomVar rv_div(RandomVar a, RandomVar b); /* approximation, use with caution */
RandomVar rv_inv(RandomVar a);

/* Statistical analysis helpers */
typedef struct CheckasmSample {
    uint64_t sum; /* batched sum of data points */
    int count;    /* number of data points in batch */
} CheckasmSample;

typedef struct CheckasmStats {
    /* With a ~12% exponential growth on the number of data points per sample,
     * 256 samples can effectively represent many billions of data points */
    CheckasmSample samples[256];
    int nb_samples;
    int next_count;
} CheckasmStats;

static inline void checkasm_stats_reset(CheckasmStats *const stats)
{
    stats->nb_samples = 0;
    stats->next_count = 1;
}

static inline void checkasm_stats_add(CheckasmStats *const stats,
                                      const CheckasmSample s)
{
    if (s.count > 0) {
        assert(stats->nb_samples < (int) ARRAY_SIZE(stats->samples));
        stats->samples[stats->nb_samples++] = s;
    }
}

/* Increase number of data points exponentially, with 1/8 = ~12% growth */
static inline void checkasm_stats_count_grow(CheckasmStats *const stats)
{
    stats->next_count = ((stats->next_count << 3) + stats->next_count + 7) >> 3;
    stats->next_count = imin(stats->next_count, 1 << 28);
}

int checkasm_stats_count_total(const CheckasmStats *stats);

typedef struct CheckasmOutliers {
    /* Total number of outliers (as a fraction of the total) */
    double outliers;

    /* Breakdown into categories */
    double low_mild, low_extreme;
    double high_mild, high_extreme;
} CheckasmOutliers;

/* `outliers` is optional */
RandomVar checkasm_stats_estimate(CheckasmStats *stats,
                                  CheckasmOutliers *outliers);

#endif /* CHECKASM_STATS_H */
