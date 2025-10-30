(function () {
  "use strict";
  window.addEventListener(
    "beforeprint",
    function () {
      for (const id in Chart.instances) {
        Chart.instances[id].resize();
      }
    },
    false,
  );

  const errorBarPlugin = (function () {
    function drawErrorBar(chart, ctx, low, high, y, height, color) {
      ctx.save();
      ctx.lineWidth = 3;
      ctx.strokeStyle = color;
      const area = chart.chartArea;
      ctx.rect(area.left, area.top, area.right - area.left, area.bottom - area.top);
      ctx.clip();
      ctx.beginPath();
      ctx.moveTo(low, y - height);
      ctx.lineTo(low, y + height);
      ctx.moveTo(low, y);
      ctx.lineTo(high, y);
      ctx.moveTo(high, y - height);
      ctx.lineTo(high, y + height);
      ctx.stroke();
      ctx.restore();
    }
    // Avoid sudden jumps in error bars when switching
    // between linear and logarithmic scale
    function conservativeError(vx, mx, now, final, scale) {
      const finalDiff = Math.abs(mx - final);
      const diff = Math.abs(vx - now);
      return diff > finalDiff ? vx + scale * finalDiff : now;
    }
    return {
      afterDatasetDraw: function (chart, easingOptions) {
        const ctx = chart.ctx;
        const easing = easingOptions.easingValue;
        chart.data.datasets.forEach(function (d, i) {
          const bars = chart.getDatasetMeta(i).data;
          const axis = chart.scales[chart.options.scales.xAxes[0].id];
          bars.forEach(function (b, j) {
            const value = axis.getValueForPixel(b._view.x);
            const final = axis.getValueForPixel(b._model.x);
            const errorBar = d.errorBars[j];
            const low = axis.getPixelForValue(value - errorBar.minus);
            const high = axis.getPixelForValue(value + errorBar.plus);
            const finalLow = axis.getPixelForValue(final - errorBar.minus);
            const finalHigh = axis.getPixelForValue(final + errorBar.plus);
            const l =
              easing === 1
                ? finalLow
                : conservativeError(b._view.x, b._model.x, low, finalLow, -1.0);
            const h =
              easing === 1
                ? finalHigh
                : conservativeError(b._view.x, b._model.x, high, finalHigh, 1.0);
            drawErrorBar(chart, ctx, l, h, b._view.y, 4, errorBar.color);
          });
        });
      },
    };
  })();

  // Formats the ticks on the X-axis on the scatter plot
  const iterFormatter = function () {
    var denom = 0;
    return function (iters, index, values) {
      if (iters == 0)
        return "";
      if (index == values.length - 1)
        return "";
      var power;
      if (iters >= 1e9) {
        denom = 1e9;
        power = "⁹";
      } else if (iters >= 1e6) {
        denom = 1e6;
        power = "⁶";
      } else if (iters >= 1e3) {
        denom = 1e3;
        power = "³";
      } else {
        denom = 1;
      }
      if (denom > 1) {
        const value = (iters / denom).toFixed();
        return String(value) + "×10" + power;
      } else {
        return String(iters);
      }
    };
  };

  const colors = ["#edc240", "#afd8f8", "#cb4b4b", "#4da74d", "#9440ed"];
  const errorColors = ["#cda220", "#8fb8d8", "#ab2b2b", "#2d872d", "#7420cd"];

  // Positions tooltips at cursor. Required for overview since the bars may
  // extend past the canvas width.
  Chart.Tooltip.positioners.cursor = function (_elems, position) {
    return position;
  };

  function axisType(logaxis) {
    return logaxis ? "logarithmic" : "linear";
  }

  /* Adds indices and full names to each report, and group */
  function processReports(reportData) {
    const tests = {};

    Object.entries(reportData.functions).forEach(([funcName, func]) => {
      /* Group by test name */
      if (!tests[func.testName])
        tests[func.testName] = {};
      const reports = tests[func.testName];
      /* Group by report name */
      if (!reports[func.reportName])
        reports[func.reportName] = { numVersions: 0, functions: {} };
      const report = reports[func.reportName];
      const funcNumber = Object.keys(report.functions).length;
      report.functions[funcName] = func;
      Object.entries(func.versions).forEach(([suffix, version]) => {
        /* Group versions by function */
        version.groups = [funcName, suffix];
        version.groupNumber = funcNumber;
        version.reportName = funcName + "_" + suffix;
        version.reportNumber = report.numVersions;
        report.numVersions += 1;
      });
    });

    return tests;
  }

  function reportSort(a, b) {
    return a.reportNumber - b.reportNumber;
  }

  function durationSort(a, b) {
    return a.adjustedTime.mode - b.adjustedTime.mode;
  }
  function reverseDurationSort(a, b) {
    return -durationSort(a, b);
  }

  const collator = new Intl.Collator(undefined, { numeric: true, sensitivity: "base" });

  // compares 2 arrays lexicographically
  function lex(aParts, bParts) {
    for (var i = 0; i < aParts.length && i < bParts.length; i++) {
      const x = aParts[i];
      const y = bParts[i];
      const comp = collator.compare(x, y);
      if (comp)
        return comp;
    }
    return aParts.length - bParts.length;
  }
  function lexicalSort(a, b) {
    return lex(a.groups, b.groups);
  }

  function reverseLexicalSort(a, b) {
    return lex(a.groups.slice().reverse(), b.groups.slice().reverse());
  }

  function timeUnits(secs) {
         if (secs < 0)      return timeUnits(-secs);
    else if (secs >= 1e9)   return [1e-9, "Gs"];
    else if (secs >= 1e6)   return [1e-6, "Ms"];
    else if (secs >= 1)     return [1, "s"];
    else if (secs >= 1e-3)  return [1e3, "ms"];
    else if (secs >= 1e-6)  return [1e6, "\u03bcs"];
    else if (secs >= 1e-9)  return [1e9, "ns"];
    else if (secs >= 1e-12) return [1e12, "ps"];
    else return [1, "s"];
  }

  function rawUnits(value) {
         if (value >= 1e9) return [1e-9, "G"];
    else if (value >= 1e6) return [1e-6, "M"];
    else if (value >= 1e3) return [1e-3, "k"];
    return [1, ""];
  }

  function formatUnit(raw, unit, precision) {
    const v = precision ? raw.toPrecision(precision) : Math.round(raw);
    const label = String(v) + " " + unit;
    return label;
  }

  function formatTime(nsecs, precision) {
    const secs = nsecs * 1e-9;
    const units = timeUnits(secs);
    const scale = units[0];
    return formatUnit(secs * scale, units[1], precision);
  }

  function formatCycles(cycles, unit, precision) {
    if (unit === "nsec") {
      return formatTime(cycles, precision);
    } else {
      const units = rawUnits(cycles);
      const scale = units[0];
      return formatUnit(cycles * scale, units[1] + unit + "s", precision);
    }
  }

  // pure function that produces the 'data' object of the overview chart
  function overviewData(state, reports) {
    const order = state.order;
    const sorter = order === "report-index" ? reportSort
                 : order === "lex"          ? lexicalSort
                 : order === "colex"        ? reverseLexicalSort
                 : order === "duration"     ? durationSort
                 : order === "rev-duration" ? reverseDurationSort
                 : reportSort;
    const sortedReports = reports.filter((r) => !state.hidden[r.groupNumber]).sort(sorter);
    const data = sortedReports.map((report) => report.adjustedTime.mode);
    const labels = sortedReports.map((report) => report.reportName);
    const upperBound = (report) => report.adjustedTime.upperCI;
    const errorBars = sortedReports.map(function (report) {
      const est = report.adjustedTime.mode;
      return {
        minus: est - report.adjustedTime.lowerCI,
        plus: report.adjustedTime.upperCI - est,
        color: errorColors[report.groupNumber % errorColors.length],
      };
    });
    const top = sortedReports.map(upperBound).reduce((a, b) => Math.max(a, b), 0);
    var scale = top;
    if (state.activeReport !== null) {
      reports.forEach(function (report) {
        if (report.reportNumber === state.activeReport)
          scale = upperBound(report);
      });
    }

    return {
      labels: labels,
      top: top,
      max: scale * 1.1,
      reports: sortedReports,
      datasets: [
        {
          borderWidth: 1,
          backgroundColor: sortedReports.map(function (report) {
            const active = report.reportNumber === state.activeReport;
            const alpha = active ? "ff" : "a0";
            const color = colors[report.groupNumber % colors.length] + alpha;
            if (active) {
              return Chart.helpers.getHoverColor(color);
            } else {
              return color;
            }
          }),
          barThickness: 16,
          barPercentage: 0.8,
          data: data,
          errorBars: errorBars,
          minBarLength: 2,
        },
      ],
    };
  }

  function inside(box, point) {
    return (
      point.x >= box.left && point.x <= box.right && point.y >= box.top && point.y <= box.bottom
    );
  }

  function overviewHover(event, elems) {
    const chart = this;
    const xAxis = chart.scales[chart.options.scales.xAxes[0].id];
    const yAxis = chart.scales[chart.options.scales.yAxes[0].id];
    const point = Chart.helpers.getRelativePosition(event, chart);
    const over = inside(xAxis, point) || inside(yAxis, point) || elems.length > 0;
    if (over)
      chart.canvas.style.cursor = "pointer";
    else
      chart.canvas.style.cursor = "default";
  }

  // Re-renders the overview after clicking/sorting
  function renderOverview(state, reports, chart) {
    const data = overviewData(state, reports);
    const xaxis = chart.options.scales.xAxes[0];
    xaxis.ticks.max = data.max;
    chart.config.data.datasets[0].backgroundColor = data.datasets[0].backgroundColor;
    chart.config.data.datasets[0].errorBars = data.datasets[0].errorBars;
    chart.config.data.datasets[0].data = data.datasets[0].data;
    chart.options.scales.xAxes[0].type = axisType(state.logaxis);
    chart.options.legend.display = state.legend;
    chart.data.labels = data.labels;
    chart.update();
  }

  function overviewClick(state, reports) {
    return function (event, elems) {
      const chart = this;
      const xAxis = chart.scales[chart.options.scales.xAxes[0].id];
      const yAxis = chart.scales[chart.options.scales.yAxes[0].id];
      const point = Chart.helpers.getRelativePosition(event, chart);
      const sorted = overviewData(state, reports).reports;

      function activateBar(index) {
        // Trying to activate active bar disables instead
        if (sorted[index].reportNumber === state.activeReport)
          state.activeReport = null;
        else
          state.activeReport = sorted[index].reportNumber;
      }

      if (inside(xAxis, point)) {
        state.activeReport = null;
        state.logaxis = !state.logaxis;
        renderOverview(state, reports, chart);
      } else if (inside(yAxis, point)) {
        const index = yAxis.getValueForPixel(point.y);
        activateBar(index);
        renderOverview(state, reports, chart);
      } else if (elems.length > 0) {
        const elem = elems[0];
        const index = elem._index;
        activateBar(index);
        state.logaxis = false;
        renderOverview(state, reports, chart);
      } else if (inside(chart.chartArea, point)) {
        state.activeReport = null;
        renderOverview(state, reports, chart);
      }
    };
  }

  // Returns listener for sort drop-down
  function overviewSort(state, reports, chart) {
    return function (event) {
      state.order = event.currentTarget.value;
      renderOverview(state, reports, chart);
    };
  }

  // Returns a formatter for the ticks on the X-axis of the overview
  function overviewTick(state) {
    return function (value, index, values) {
      const label = formatTime(value);
      if (state.logaxis) {
        const remain = Math.round(value / Math.pow(10, Math.floor(Chart.helpers.log10(value))));
        if (index === values.length - 1) {
          // Draw endpoint if we don't span a full order of magnitude
          if (values[index] / values[1] < 10)
            return label;
          else
            return "";
        }
        if (remain === 1)
          return label;
        return "";
      } else {
        // Don't show the right endpoint
        if (index === values.length - 1)
          return "";
        return label;
      }
    };
  }

  function mkOverview(reports) {
    const canvas = document.createElement("canvas");

    const state = {
      logaxis: false,
      activeReport: null,
      order: "index",
      hidden: {},
      legend: false,
    };

    const data = overviewData(state, reports);
    const chart = new Chart(canvas.getContext("2d"), {
      type: "horizontalBar",
      data: data,
      plugins: [errorBarPlugin],
      options: {
        onHover: overviewHover,
        onClick: overviewClick(state, reports),
        onResize: function (chart, size) {
          if (size.width < 800) {
            chart.options.scales.yAxes[0].ticks.mirror = true;
            chart.options.scales.yAxes[0].ticks.padding = -10;
            chart.options.scales.yAxes[0].ticks.fontColor = "#000";
          } else {
            chart.options.scales.yAxes[0].ticks.fontColor = "#666";
            chart.options.scales.yAxes[0].ticks.mirror = false;
            chart.options.scales.yAxes[0].ticks.padding = 0;
          }
        },
        elements: {
          rectangle: {
            borderWidth: 2,
          },
        },
        scales: {
          yAxes: [
            {
              ticks: {
                // make sure we draw the ticks above the error bars
                z: 2,
              },
            },
          ],
          xAxes: [
            {
              display: true,
              type: axisType(state.logaxis),
              ticks: {
                autoSkip: false,
                min: 0,
                max: data.top * 1.1,
                minRotation: 0,
                maxRotation: 0,
                callback: overviewTick(state),
              },
            },
          ],
        },
        responsive: true,
        maintainAspectRatio: false,
        legend: {
          display: state.legend,
          position: "right",
          onLeave: function () {
            chart.canvas.style.cursor = "default";
          },
          onHover: function () {
            chart.canvas.style.cursor = "pointer";
          },
          onClick: function (_event, item) {
            // toggle hidden
            state.hidden[item.groupNumber] = !state.hidden[item.groupNumber];
            renderOverview(state, reports, chart);
          },
          labels: {
            boxWidth: 12,
            generateLabels: function () {
              const groups = [];
              const groupNames = [];
              reports.forEach(function (report) {
                const index = groups.indexOf(report.groupNumber);
                if (index === -1) {
                  groups.push(report.groupNumber);
                  const groupName = report.groups.slice(0, report.groups.length - 1).join(" / ");
                  groupNames.push(groupName);
                }
              });
              return groups.map(function (groupNumber, index) {
                const color = colors[groupNumber % colors.length];
                return {
                  text: groupNames[index],
                  fillStyle: color,
                  hidden: state.hidden[groupNumber],
                  groupNumber: groupNumber,
                };
              });
            },
          },
        },
        tooltips: {
          position: "cursor",
          callbacks: {
            label: function (item) {
              return formatTime(item.xLabel, 3);
            },
          },
        },
        title: {
          display: false,
          text: "Chart.js Horizontal Bar Chart",
        },
      },
    });
    document
      .getElementById("sort-overview")
      .addEventListener("change", overviewSort(state, reports, chart));
    const toggle = document.getElementById("legend-toggle");
    toggle.addEventListener("mouseup", function () {
      state.legend = !state.legend;
      if (state.legend)
        toggle.classList.add("right");
      else
        toggle.classList.remove("right");
      renderOverview(state, reports, chart);
    });
    return canvas;
  }

  function mkKDE(report) {
    const canvas = document.createElement("canvas");
    const rawCycles = report.rawCycles;
    const mean = rawCycles.mean;
    const mu = rawCycles.logMean;
    const sigma = Math.sqrt(rawCycles.logVar);
    const units = rawUnits(mean);
    const scale = units[0];

    const numPoints = 100;
    const threshold = 3;
    const minVal = Math.exp(mu - threshold * sigma);
    const maxVal = Math.exp(mu + threshold * sigma);
    const step = (maxVal - minVal) / (numPoints - 1);

    let data = [];
    for (let i = 0; i < numPoints; i++) {
      const x = minVal + i * step;
      const y = Math.exp(-0.5 * Math.pow((Math.log(x) - mu) / sigma, 2));
      data.push({ x: x * scale, y: y });
    }

    const chart = new Chart(canvas.getContext("2d"), {
      type: "line",
      data: {
        datasets: [
          {
            label: "KDE",
            borderColor: colors[0],
            borderWidth: 2,
            backgroundColor: "#00000000",
            data: data,
            hoverBorderWidth: 1,
            pointHitRadius: 8,
          },
          {
            label: "mean",
          },
        ],
      },
      plugins: [
        {
          afterDraw: function (chart) {
            const ctx = chart.ctx;
            const area = chart.chartArea;
            const axis = chart.scales[chart.options.scales.xAxes[0].id];
            const value = axis.getPixelForValue(mean * scale);
            ctx.save();
            ctx.strokeStyle = colors[1];
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.moveTo(value, area.top);
            ctx.lineTo(value, area.bottom);
            ctx.stroke();
            ctx.restore();
          },
        },
      ],
      options: {
        title: {
          display: true,
          text: report.groups.join(" / ") + " — probability density",
        },
        elements: {
          point: {
            radius: 0,
            hitRadius: 0,
          },
        },
        scales: {
          xAxes: [
            {
              display: true,
              type: "linear",
              scaleLabel: {
                display: true,
                labelString: rawCycles.unit + "s",
              },
              ticks: {
                min: minVal * scale,
                max: maxVal * scale,
                callback: function (value, index, values) {
                  // Don't show endpoints
                  if (index === 0 || index === values.length - 1)
                    return "";
                  const str = String(value) + " " + units[1];
                  return str;
                },
              },
            },
          ],
          yAxes: [
            {
              display: false,
              type: "linear",
            },
          ],
        },
        responsive: true,
        legend: {
          display: false,
          position: "right",
        },
        tooltips: {
          mode: "nearest",
          callbacks: {
            title: function () {
              return "";
            },
            label: function (item) {
              return formatUnit(item.xLabel, units[1], 3);
            },
          },
        },
        hover: {
          intersect: false,
        },
      },
    });
    return canvas;
  }

  function mkScatter(report) {
    const canvas = document.createElement("canvas");
    const cycles = report.rawData.map((x) => x.cycles);
    const iters = report.rawData.map((x) => x.iters);
    const lastIter = iters[iters.length - 1];
    const slope = report.regressionSlope;
    const dataPoints = cycles.map(function (time, i) {
      return {
        x: iters[i],
        y: time,
      };
    });
    const formatter = iterFormatter();
    var title = slope.unit + "s per iteration";
    if (report.numBenchmarks > 1)
      title += " (last run only)";
    const chart = new Chart(canvas.getContext("2d"), {
      type: "scatter",
      data: {
        datasets: [
          {
            data: dataPoints,
            label: "scatter",
            borderWidth: 2,
            pointHitRadius: 8,
            borderColor: colors[1],
            backgroundColor: "#fff",
          },
          {
            data: [
              { x: 0, y: 0 },
              { x: lastIter, y: slope.mode * lastIter },
            ],
            label: "regression",
            type: "line",
            backgroundColor: "#00000000",
            borderColor: colors[0],
            pointRadius: 0,
          },
          {
            data: [
              { x: 0, y: 0 },
              { x: lastIter, y: slope.lowerCI * lastIter },
            ],
            label: "lower",
            type: "line",
            fill: 1,
            borderWidth: 0,
            pointRadius: 0,
            borderColor: "#00000000",
            backgroundColor: colors[0] + "33",
          },
          {
            data: [
              { x: 0, y: 0 },
              { x: lastIter, y: slope.upperCI * lastIter },
            ],
            label: "upper",
            type: "line",
            fill: 1,
            borderWidth: 0,
            borderColor: "#00000000",
            pointRadius: 0,
            backgroundColor: colors[0] + "33",
          },
        ],
      },
      options: {
        title: {
          display: true,
          text: report.groups.join(" / ") + " — " + title,
        },
        scales: {
          yAxes: [
            {
              display: true,
              type: "linear",
              scaleLabel: {
                display: false,
                labelString: slope.unit + "s",
              },
              ticks: {
                callback: function (value, index, values) {
                  return formatCycles(value, slope.unit, 3);
                },
              },
            },
          ],
          xAxes: [
            {
              display: true,
              type: "linear",
              scaleLabel: {
                display: false,
                labelString: "Iterations",
              },
              ticks: {
                callback: formatter,
                max: lastIter,
              },
            },
          ],
        },
        legend: {
          display: false,
        },
        tooltips: {
          callbacks: {
            label: function (ttitem, ttdata) {
              const iters = ttitem.xLabel;
              const cycles = ttitem.yLabel;
              return (
                formatCycles(cycles, slope.unit, 3) + " / " + iters.toLocaleString() + " iters"
              );
            },
          },
        },
      },
    });
    return canvas;
  }

  // Create an HTML Element with attributes and child nodes
  function elem(tag, props, children) {
    const node = document.createElement(tag);
    if (children) {
      children.forEach(function (child) {
        if (typeof child === "string") {
          const txt = document.createTextNode(child);
          node.appendChild(txt);
        } else {
          node.appendChild(child);
        }
      });
    }
    Object.assign(node, props);
    return node;
  }

  function mkTable(report) {
    const timep4 = function (t) {
      return formatTime(t, 3);
    };
    const cyclesp4 = function (c) {
      return formatCycles(c, report.rawCycles.unit, 3);
    };
    const idp4 = function (t) {
      return t.toPrecision(3);
    };
    var rows = [
      Object.assign(
        {
          label: "Adjusted cycles",
          formatter: cyclesp4,
        },
        report.adjustedCycles,
      ),
      Object.assign(
        {
          label: "Adjusted time",
          formatter: timep4,
        },
        report.adjustedTime,
      ),
      Object.assign(
        {
          label: "Raw cycles",
          formatter: cyclesp4,
        },
        report.rawCycles,
      ),
      Object.assign(
        {
          label: "Raw time",
          formatter: timep4,
        },
        report.rawTime,
      ),
    ];
    if (report.ratio) {
      rows.push(Object.assign(
        {
          label: "Ratio (vs ref)",
          formatter: idp4,
        },
        report.ratio,
      ));
    }
    return elem(
      "table",
      {
        className: "analysis",
      },
      [
        elem("thead", {}, [
          elem("tr", {}, [
            elem("th"),
            elem(
              "th",
              {
                className: "low",
                title: "95% lower confidence bound",
              },
              ["lower bound"],
            ),
            elem("th", {}, ["estimate"]),
            elem(
              "th",
              {
                className: "high",
                title: "95% upper confidence bound",
              },
              ["upper bound"],
            ),
          ]),
        ]),
        elem(
          "tbody",
          {},
          rows.map(function (row) {
            return elem("tr", {}, [
              elem("td", {}, [row.label]),
              elem("td", { className: "low" }, [row.formatter(row.lowerCI, 4)]),
              elem("td", {}, [row.formatter(row.mode)]),
              elem("td", { className: "high" }, [row.formatter(row.upperCI, 4)]),
            ]);
          }),
        ),
      ],
    );
  }

  function slugify(groups) {
    return groups
      .join("_")
      .toLowerCase()
      .replace(/\s+/g, "_") // Replace spaces with _
      .replace(/[^\w\-]+/g, ""); // Remove all non-word chars
  }

  document.addEventListener(
    "DOMContentLoaded",
    function () {
      const reportJSON = JSON.parse(document.getElementById("report-data").text);
      const reportData = processReports(reportJSON);
      const version = "checkasm v" + reportJSON.checkasmVersion;
      document.getElementById("checkasm-version").textContent = version;

      const overview = document.getElementById("overview-charts");
      const reports = document.getElementById("reports");
      const overviewLineHeight = 16 * 1.25;

      Object.entries(reportData).forEach(([testName, testData], testNumber) => {
        const tid = slugify([testName]);
        const testOverview = elem("div", { id: tid }, [
          elem("h2", {}, [elem("a", { href: "#" + tid }, [testName])]),
        ]);
        overview.appendChild(testOverview);

        Object.entries(testData).forEach(([reportName, report]) => {
          const height = overviewLineHeight * report.numVersions + 36;
          testOverview.appendChild(
            elem("details", { class: "group-summary", open }, [
              elem("summary", {}, [reportName]),
              elem("p", { style: "height: " + String(height) + "px" }, [
                mkOverview(
                  Object.values(report.functions).flatMap((f) => Object.values(f.versions)),
                ),
              ]),
            ]),
          );

          const rid = slugify([testName, reportName]);
          const toggleReport = elem("button", {}, ["Expand All"]);
          const reportDiv = elem("div", { id: rid }, [
            elem("h1", {}, [elem("a", { href: "#" + rid }, [[testName, reportName].join(" / ")])]),
            toggleReport,
          ]);
          reports.appendChild(reportDiv);

          Object.entries(report.functions).forEach(([funcName, func]) => {
            const body = elem("p", {}, []);
            const details = elem("details", { className: "report-details" }, [
              elem("summary", {}, [funcName]), body
            ]);
            reportDiv.appendChild(details);

            /* Generate details lazily on demand */
            details.addEventListener("toggle", () => {
              if (details.open && body.childElementCount === 0) {
                Object.values(func.versions).forEach((version) => {
                  const id = slugify([testName, reportName, version.reportName]);
                  body.appendChild(
                    elem("h3", { id: id }, [elem("a", { href: "#" + id }, [version.reportName])]),
                  );
                  body.appendChild(elem("div", { className: "kde" }, [mkKDE(version)]));
                  if (version.rawData) {
                    body.appendChild(elem("div", { className: "scatter" }, [
                      mkScatter(version)
                    ]));
                  }
                  body.appendChild(mkTable(version));
                });
              }
            });
          });

          const reportDetails = reportDiv.querySelectorAll("details");
          toggleReport.state = false;
          toggleReport.addEventListener("click", () => {
            toggleReport.state = !toggleReport.state;
            toggleReport.textContent = toggleReport.state ? "Collapse All" : "Expand All";
            reportDetails.forEach((d) => (d.open = toggleReport.state));
          });
        });
      });

      const overviewDetails = overview.querySelectorAll("details");
      const toggleOverview = document.getElementById("toggleOverview");
      toggleOverview.state = true;
      toggleOverview.addEventListener("click", () => {
        toggleOverview.state = !toggleOverview.state;
        toggleOverview.textContent = toggleOverview.state ? "Collapse All" : "Expand All";
        overviewDetails.forEach((d) => (d.open = toggleOverview.state));
      });
    },
    false,
  );
})();
