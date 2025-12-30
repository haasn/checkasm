/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "checkasm", "index.html", [
    [ "Introduction", "index.html", "index" ],
    [ "Getting Started", "getting_started.html", [
      [ "Installation", "getting_started.html#installation", [
        [ "Meson using submodules (recommended)", "getting_started.html#meson_submodules", null ],
        [ "Meson using wrap files (alternative)", "getting_started.html#meson_wrap", null ],
        [ "Manual Installation", "getting_started.html#manual_installation", null ]
      ] ],
      [ "Quick Start Example", "getting_started.html#quick_start", [
        [ "1. Prerequisites", "getting_started.html#quick_start1", null ],
        [ "2. Write the Test", "getting_started.html#quick_start2", null ],
        [ "3. Build and Run", "getting_started.html#quick_start3", null ]
      ] ],
      [ "Command-Line Options", "getting_started.html#options", null ],
      [ "Next Steps", "getting_started.html#getting_started_next_steps", null ]
    ] ],
    [ "Integration Guide", "integration.html", [
      [ "CPU Flags", "integration.html#cpu_flags", [
        [ "Defining CPU Flags", "integration.html#defining_cpu_flags", null ],
        [ "Extra CPU Flags", "integration.html#extra_cpu_flags", null ]
      ] ],
      [ "Selecting Functions", "integration.html#selecting_functions", [
        [ "Strategy 1: Mask Callback", "integration.html#mask_callback", null ],
        [ "Strategy 2: Direct Getters", "integration.html#direct_getters", null ]
      ] ],
      [ "Organizing Multiple Tests", "integration.html#organizing_tests", null ],
      [ "Next Steps", "integration.html#integration_next_steps", null ]
    ] ],
    [ "Writing Tests", "writing_tests.html", [
      [ "Basic Test Structure", "writing_tests.html#test_structure", null ],
      [ "API Naming Conventions", "writing_tests.html#naming_conventions", [
        [ "Modern API (Recommended)", "writing_tests.html#modern_api", null ],
        [ "Legacy/Short API", "writing_tests.html#legacy_api", null ]
      ] ],
      [ "Best Practices", "writing_tests.html#best_practices", [
        [ "Buffer Allocation", "writing_tests.html#bp_buffer_allocation", null ],
        [ "Buffer Initialization", "writing_tests.html#bp_buffer_init", null ],
        [ "Testing Multiple Configurations", "writing_tests.html#bp_test_loops", null ],
        [ "Using Random Parameters", "writing_tests.html#bp_random_params", null ],
        [ "Organizing Reports", "writing_tests.html#bp_reporting", null ]
      ] ],
      [ "Common Test Patterns", "writing_tests.html#common_patterns", [
        [ "2D Buffer Processing", "writing_tests.html#pattern_2d", null ],
        [ "State-Based Functions", "writing_tests.html#pattern_state", null ],
        [ "Multiple Outputs", "writing_tests.html#pattern_multi_output", null ],
        [ "Custom Input Generation", "writing_tests.html#pattern_custom_input", null ]
      ] ],
      [ "Advanced Topics", "writing_tests.html#advanced_topics", [
        [ "Floating-Point Comparison", "writing_tests.html#adv_float", null ],
        [ "Padding and Over-Write Detection", "writing_tests.html#adv_padding", null ],
        [ "Benchmarking Multiple Configurations", "writing_tests.html#bench_multiple", null ],
        [ "Multi-Bitdepth Testing", "writing_tests.html#adv_bitdepth", null ],
        [ "Custom Failure Reporting", "writing_tests.html#adv_failure", null ],
        [ "MMX Functions (x86)", "writing_tests.html#adv_mmx", null ]
      ] ],
      [ "Tips and Tricks", "writing_tests.html#tips_tricks", [
        [ "Deterministic Testing", "writing_tests.html#tips_deterministic", null ],
        [ "Selective Testing", "writing_tests.html#tips_selective", null ],
        [ "Verbose Output", "writing_tests.html#tips_verbose", null ],
        [ "Benchmarking Tips", "writing_tests.html#tips_bench", null ],
        [ "Helper Macros", "writing_tests.html#tips_helpers", null ]
      ] ],
      [ "Next Steps", "writing_tests.html#tests_next_steps", null ]
    ] ],
    [ "Benchmarking", "benchmarking.html", [
      [ "Basics", "benchmarking.html#bench_basic", [
        [ "Benchmark Workflow", "benchmarking.html#bench_workflow", null ],
        [ "Running Benchmarks", "benchmarking.html#bench_cli", null ],
        [ "Exporting Results", "benchmarking.html#bench_export", null ]
      ] ],
      [ "Statistical Methodology", "benchmarking.html#bench_methodology", [
        [ "Log-Normal Distribution Modeling", "benchmarking.html#bench_lognormal", null ],
        [ "Linear Regression", "benchmarking.html#bench_regression", null ],
        [ "Geometric Mean for Multiple Runs", "benchmarking.html#bench_geometric", null ],
        [ "Overhead Correction", "benchmarking.html#bench_overhead", null ]
      ] ],
      [ "Best Practices", "benchmarking.html#bench_best_practices", [
        [ "System State", "benchmarking.html#bp_system_state", null ],
        [ "Cache Alignment", "benchmarking.html#bp_alignment", null ],
        [ "Buffer Alternation", "benchmarking.html#bp_alternating", null ],
        [ "Realistic Test Data", "benchmarking.html#bp_realistic", null ],
        [ "Choosing Configurations", "benchmarking.html#bp_configurations", null ]
      ] ],
      [ "Interpreting Results", "benchmarking.html#bench_interpreting", [
        [ "Understanding Output", "benchmarking.html#interp_output", null ],
        [ "High Variance", "benchmarking.html#interp_variance", null ],
        [ "Comparing Implementations", "benchmarking.html#interp_comparison", null ],
        [ "Regression Detection", "benchmarking.html#interp_regression", null ]
      ] ],
      [ "Advanced Topics", "benchmarking.html#bench_advanced", [
        [ "Microbenchmarking Pitfalls", "benchmarking.html#adv_microbench", null ],
        [ "Platform Considerations", "benchmarking.html#adv_platform", [
          [ "Timer Resolution", "benchmarking.html#adv_timer", null ],
          [ "Frequency Scaling", "benchmarking.html#adv_freq_scaling", null ],
          [ "Cross-Platform Comparison", "benchmarking.html#adv_cross_platform", null ]
        ] ],
        [ "HTML Report Deep Dive", "benchmarking.html#adv_html", [
          [ "Kernel Density Estimate (left chart)", "benchmarking.html#adv_kde_regression", null ],
          [ "Raw Measurements (right chart)", "benchmarking.html#adv_raw_measurements", null ],
          [ "Metrics Table", "benchmarking.html#adv_metrics", null ]
        ] ]
      ] ],
      [ "Tips and Tricks", "benchmarking.html#bench_tips", [
        [ "Reproducible Benchmarks", "benchmarking.html#tips_reproducible", null ]
      ] ]
    ] ],
    [ "API Reference", "files.html", "files" ]
  ] ]
];

var NAVTREEINDEX =
[
"attributes_8h.html"
];

var SYNCONMSG = 'click to disable panel synchronization';
var SYNCOFFMSG = 'click to enable panel synchronization';
var LISTOFALLMEMBERS = 'List of all members';