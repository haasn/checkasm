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

static inline double checkasm_stddev(const CheckasmVar x)
{
    return exp(x.lmean + 0.5 * x.lvar) * sqrt(exp(x.lvar) - 1.0);
}

/* Returns the point estimate of a random variable at the given sigma level;
 * sigma=0.0 gives the mean, sigma=1.0 gives the 1 sigma upper bound, etc. */
static inline double checkasm_sample(const CheckasmVar x, const double sigma)
{
    return exp(x.lmean + sigma * sqrt(x.lvar));
}

static inline double checkasm_mean(const CheckasmVar x)
{
    return checkasm_sample(x, 0.0);
}

static inline CheckasmVar checkasm_var_const(double x)
{
    return (CheckasmVar) { log(x), 0.0 };
}

/* Assumes independent random variables */
CheckasmVar checkasm_var_scale(CheckasmVar a, double s);
CheckasmVar checkasm_var_pow(CheckasmVar a, double exp);
CheckasmVar checkasm_var_add(CheckasmVar a, CheckasmVar b); /* approximation */
CheckasmVar checkasm_var_sub(CheckasmVar a, CheckasmVar b); /* approximation */
CheckasmVar checkasm_var_mul(CheckasmVar a, CheckasmVar b);
CheckasmVar checkasm_var_div(CheckasmVar a, CheckasmVar b);
CheckasmVar checkasm_var_inv(CheckasmVar a);

/* Statistical analysis helpers */
typedef struct CheckasmSample {
    uint64_t sum; /* batched sum of data points */
    int      count; /* number of data points in batch */
} CheckasmSample;

typedef struct CheckasmStats {
    /* With an exponential growth on the number of data points per sample,
     * even a small number of samples can effectively represent many billions
     * of data points */
    CheckasmSample samples[1024];
    int            nb_samples;
    int            next_count;
} CheckasmStats;

static inline void checkasm_stats_reset(CheckasmStats *const stats)
{
    stats->nb_samples = 0;
    stats->next_count = 1;
}

static inline void checkasm_stats_add(CheckasmStats *const stats, const CheckasmSample s)
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

typedef struct CheckasmDistribution {
    double min, max;
    double median;
    double q1, q3; /* 25% and 75% quantiles */

    /* Total number of outliers (as a fraction of the total) */
    double outliers;

    /* Breakdown into categories */
    double low_mild, low_extreme;
    double high_mild, high_extreme;
} CheckasmDistribution;

CheckasmVar checkasm_stats_estimate(CheckasmStats *stats);

#endif /* CHECKASM_STATS_H */
