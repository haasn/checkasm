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

#include <math.h>
#include <stdlib.h>

#include "stats.h"

CheckasmVar checkasm_var_scale(CheckasmVar a, double s)
{
    /* = checkasm_var_mul(a, checkasm_var_const(b)) */
    return (CheckasmVar) {
        .mean = a.mean * s,
        .var  = a.var * s * s,
    };
}

CheckasmVar checkasm_var_add(const CheckasmVar a, const CheckasmVar b)
{
    return (CheckasmVar) {
        .mean = a.mean + b.mean,
        .var  = a.var + b.var,
    };
}

CheckasmVar checkasm_var_sub(CheckasmVar a, CheckasmVar b)
{
    return (CheckasmVar) {
        .mean = a.mean - b.mean,
        .var  = a.var + b.var,
    };
}

CheckasmVar checkasm_var_mul(CheckasmVar a, CheckasmVar b)
{
    return (CheckasmVar) {
        .mean = a.mean * b.mean,
        .var  = a.var * b.var + a.var * b.mean * b.mean + b.var * a.mean * a.mean,
    };
}

CheckasmVar checkasm_var_inv(CheckasmVar a)
{
    /* Approximate using first-order Taylor expansion */
    const double inv_mean  = 1.0 / a.mean;
    const double inv_mean2 = inv_mean * inv_mean;
    return (CheckasmVar) {
        .mean = inv_mean,
        .var  = a.var * inv_mean2 * inv_mean2,
    };
}

CheckasmVar checkasm_var_div(CheckasmVar a, CheckasmVar b)
{
    return checkasm_var_mul(a, checkasm_var_inv(b));
}

static double sample_mean(const CheckasmSample s)
{
    return (double) s.sum / s.count;
}

/* Get the sample corresponding to a true count position */
static CheckasmSample get_sample(const CheckasmStats *const stats, int position)
{
    int seen = 0;
    for (int i = 0; i < stats->nb_samples; i++) {
        const CheckasmSample s = stats->samples[i];
        if (seen + s.count > position)
            return s;
        seen += s.count;
    }

    return (CheckasmSample) { 0, 0 };
}

/* Compare by mean value */
static int cmp_samples(const void *a, const void *b)
{
    const CheckasmSample sa = *(const CheckasmSample *) a;
    const CheckasmSample sb = *(const CheckasmSample *) b;
    const uint64_t       xa = sa.sum * sb.count, xb = sb.sum * sa.count;
    return (xa < xb) ? -1 : (xa > xb);
}

int checkasm_stats_count_total(const CheckasmStats *const stats)
{
    int total = 0;
    for (int i = 0; i < stats->nb_samples; i++)
        total += stats->samples[i].count;
    return total;
}

static CheckasmVar var_est(uint64_t sum, double sum2, int count)
{
    const double mean = (double) sum / count;
    const double var  = sum2 / count - mean * mean;
    return (CheckasmVar) { mean, var };
}

CheckasmVar checkasm_stats_estimate(CheckasmStats *const stats)
{
    if (!stats->nb_samples)
        return (CheckasmVar) { 0.0, 0.0 };

    const int total_count = checkasm_stats_count_total(stats);

    /* Sort all samples and get the Q1 and Q3 values */
    qsort(stats->samples, stats->nb_samples, sizeof(CheckasmSample), cmp_samples);
    const int idx_q1 = ((total_count - 1) * 1) / 4;
    const int idx_q3 = ((total_count - 1) * 3) / 4;

    const double q1  = sample_mean(get_sample(stats, idx_q1));
    const double q3  = sample_mean(get_sample(stats, idx_q3));
    const double iqr = q3 - q1;
    assert(iqr >= 0.0);

    /* Define boxplot thresholds */
    const double lo_mild = q1 - 1.5 * iqr;
    const double hi_mild = q3 + 1.5 * iqr;

    /* Classify and accumulate */
    uint64_t sum_raw = 0, sum_trim = 0;
    double   sum2_raw = 0.0, sum2_trim = 0.0;
    int      nb_trim = 0;

    for (int i = 0; i < stats->nb_samples; i++) {
        const CheckasmSample s = stats->samples[i];
        const double         x = sample_mean(s);
        sum_raw += s.sum;
        sum2_raw += s.sum * x;

        /* Reject outliers */
        if (x >= lo_mild && x <= hi_mild) {
            sum_trim += s.sum;
            sum2_trim += s.sum * x;
            nb_trim += s.count;
        }
    }

    assert(nb_trim > 0);
    const CheckasmVar raw  = var_est(sum_raw, sum2_raw, total_count);
    const CheckasmVar trim = var_est(sum_trim, sum2_trim, nb_trim);
    return (CheckasmVar) { trim.mean, raw.var };
}
