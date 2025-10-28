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
        .lmean = a.lmean + log(s),
        .lvar  = a.lvar,
    };
}

CheckasmVar checkasm_var_pow(CheckasmVar a, double exp)
{
    return (CheckasmVar) {
        .lmean = a.lmean * exp,
        .lvar  = a.lvar * exp * exp,
    };
}

CheckasmVar checkasm_var_add(const CheckasmVar a, const CheckasmVar b)
{
    /* Approximation assuming independent log-normal distributions */
    const double ma = exp(a.lmean + 0.5 * a.lvar);
    const double mb = exp(b.lmean + 0.5 * b.lvar);
    const double va = (exp(a.lvar) - 1.0) * exp(2.0 * a.lmean + a.lvar);
    const double vb = (exp(b.lvar) - 1.0) * exp(2.0 * b.lmean + b.lvar);
    const double m  = ma + mb;
    const double v  = va + vb;
    return (CheckasmVar) {
        .lmean = log(m * m / sqrt(v + m * m)),
        .lvar  = log(1.0 + v / (m * m)),
    };
}

CheckasmVar checkasm_var_sub(CheckasmVar a, CheckasmVar b)
{
    const double ma = exp(a.lmean + 0.5 * a.lvar);
    const double mb = exp(b.lmean + 0.5 * b.lvar);
    const double va = (exp(a.lvar) - 1.0) * exp(2.0 * a.lmean + a.lvar);
    const double vb = (exp(b.lvar) - 1.0) * exp(2.0 * b.lmean + b.lvar);
    const double m  = fmax(ma - mb, 1e-30); /* avoid negative mean */
    const double v  = va + vb;
    return (CheckasmVar) {
        .lmean = log(m * m / sqrt(v + m * m)),
        .lvar  = log(1.0 + v / (m * m)),
    };
}

CheckasmVar checkasm_var_mul(CheckasmVar a, CheckasmVar b)
{
    return (CheckasmVar) {
        .lmean = a.lmean + b.lmean,
        .lvar  = a.lvar + b.lvar,
    };
}

CheckasmVar checkasm_var_inv(CheckasmVar a)
{
    return (CheckasmVar) {
        .lmean = -a.lmean,
        .lvar  = a.lvar,
    };
}

CheckasmVar checkasm_var_div(CheckasmVar a, CheckasmVar b)
{
    return (CheckasmVar) {
        .lmean = a.lmean - b.lmean,
        .lvar  = a.lvar + b.lvar,
    };
}

static double sample_mean(const CheckasmSample s)
{
    return (double) s.sum / s.count;
}

int checkasm_stats_count_total(const CheckasmStats *const stats)
{
    int total = 0;
    for (int i = 0; i < stats->nb_samples; i++)
        total += stats->samples[i].count;
    return total;
}

CheckasmVar checkasm_stats_estimate(CheckasmStats *const stats)
{
    if (!stats->nb_samples)
        return checkasm_var_const(0.0);

    /* Compute mean and variance */
    double sum = 0.0, sum2 = 0.0;
    for (int i = 0; i < stats->nb_samples; i++) {
        const CheckasmSample s = stats->samples[i];
        const double         x = log(sample_mean(s));
        sum += x * s.count;
        sum2 += x * x * s.count;
    }

    const int    count = checkasm_stats_count_total(stats);
    const double mean  = sum / count;
    return (CheckasmVar) {
        .lmean = mean,
        .lvar  = sum2 / count - mean * mean,
    };
}

static inline CheckasmVar normal_var(double mean, double var)
{
    double lvar = log(1.0 + var / (mean * mean));
    return (CheckasmVar) {
        .lmean = log(mean) - 0.5 * lvar,
        .lvar  = lvar,
    };
}

CheckasmRegression checkasm_stats_regress(CheckasmStats *stats)
{
    const int n = stats->nb_samples;
    if (n <= 2) {
        return (CheckasmRegression) { checkasm_var_const(0.0), checkasm_var_const(0.0) };
    }

    /* Least squares linear regression to find slope through origin */
    double sum_xy = 0.0, sum_x2 = 0.0, sum_y2 = 0.0;
    for (int i = 0; i < stats->nb_samples; i++) {
        const CheckasmSample s = stats->samples[i];

        const double x = s.count;
        const double y = s.sum;
        sum_x2 += x * x;
        sum_xy += x * y;
        sum_y2 += y * y;
    };

    const double slope = sum_xy / sum_x2;

    /* Compute residual variance */
    double residual = 0.0;
    for (int i = 0; i < stats->nb_samples; i++) {
        const CheckasmSample s = stats->samples[i];

        const double x   = s.count;
        const double y   = s.sum;
        const double res = y - slope * x;
        residual += res * res;
    }

    const double s2        = residual / (n - 1);
    const double slope_var = s2 / sum_x2;
    const double r2        = (sum_xy * sum_xy) / (sum_x2 * sum_y2);
    const double r2_var    = 4.0 * r2 * (1.0 - r2) * (1.0 - r2) / (n - 2);
    return (CheckasmRegression) {
        .slope = normal_var(slope, slope_var),
        .r2    = normal_var(r2, r2_var),
    };
}
