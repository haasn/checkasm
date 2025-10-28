(function() {
  'use strict';
  window.addEventListener('beforeprint', function() {
    for (var id in Chart.instances) {
      Chart.instances[id].resize();
    }
  }, false);

  var errorBarPlugin = (function () {
    function drawErrorBar(chart, ctx, low, high, y, height, color) {
      ctx.save();
      ctx.lineWidth = 3;
      ctx.strokeStyle = color;
      var area = chart.chartArea;
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
      var finalDiff = Math.abs(mx - final);
      var diff = Math.abs(vx - now);
      return (diff > finalDiff) ? vx + scale * finalDiff : now;
    }
    return {
      afterDatasetDraw: function(chart, easingOptions) {
        var ctx = chart.ctx;
        var easing = easingOptions.easingValue;
        chart.data.datasets.forEach(function(d, i) {
          var bars = chart.getDatasetMeta(i).data;
          var axis = chart.scales[chart.options.scales.xAxes[0].id];
          bars.forEach(function(b, j) {
            var value = axis.getValueForPixel(b._view.x);
            var final = axis.getValueForPixel(b._model.x);
            var errorBar = d.errorBars[j];
            var low = axis.getPixelForValue(value - errorBar.minus);
            var high = axis.getPixelForValue(value + errorBar.plus);
            var finalLow = axis.getPixelForValue(final - errorBar.minus);
            var finalHigh = axis.getPixelForValue(final + errorBar.plus);
            var l = easing === 1 ? finalLow :
              conservativeError(b._view.x, b._model.x, low,
                finalLow, -1.0);
            var h = easing === 1 ? finalHigh :
              conservativeError(b._view.x, b._model.x,
                high, finalHigh, 1.0);
            drawErrorBar(chart, ctx, l, h, b._view.y, 4, errorBar.color);
          });
        });
      },
    };
  })();

  // Formats the ticks on the X-axis on the scatter plot
  var iterFormatter = function() {
    var denom = 0;
    return function(iters, index, values) {
      if (iters == 0) {
        return '';
      }
      if (index == values.length - 1) {
        return '';
      }
      var power;
      if (iters >= 1e9) {
        denom = 1e9;
        power = '⁹';
      } else if (iters >= 1e6) {
        denom = 1e6;
        power = '⁶';
      } else if (iters >= 1e3) {
        denom = 1e3;
        power = '³';
      } else {
        denom = 1;
      }
      if (denom > 1) {
        var value = (iters / denom).toFixed();
        return String(value) + '×10' + power;
      } else {
        return String(iters);
      }
    };
  };

  var colors = ["#edc240", "#afd8f8", "#cb4b4b", "#4da74d", "#9440ed"];
  var errorColors = ["#cda220", "#8fb8d8", "#ab2b2b", "#2d872d", "#7420cd"];


  // Positions tooltips at cursor. Required for overview since the bars may
  // extend past the canvas width.
  Chart.Tooltip.positioners.cursor = function(_elems, position) {
    return position;
  }

  function axisType(logaxis) {
    return logaxis ? 'logarithmic' : 'linear';
  }

  /* Adds indices and full names to each report, and group */
  function processReports(reportData) {
    var tests = {};

    Object.entries(reportData.functions).forEach(([funcName, func]) => {
      /* Group by test name */
      if (!tests[func.testName])
        tests[func.testName] = {};
      var reports = tests[func.testName];
      /* Group by report name */
      if (!reports[func.reportName])
        reports[func.reportName] = { numVersions: 0, functions: {} };
      var report = reports[func.reportName];
      Object.entries(func.versions).forEach(([suffix, version]) => {
        /* Group versions by function */
        version.groups = [funcName, suffix];
        version.groupNumber = Object.keys(report.functions).length;
        version.reportName = funcName + '_' + suffix;
        version.reportNumber = report.numVersions;
        report.numVersions += 1;
      });
      report.functions[funcName] = func;
    });

    return tests;
  }

  function reportSort(a, b) {
    return a.reportNumber - b.reportNumber;
  }

  function durationSort(a, b) {
    return a.time.estPoint - b.time.estPoint;
  }
  function reverseDurationSort(a,b) {
    return -durationSort(a,b);
  }

  const collator = new Intl.Collator(undefined, {numeric: true, sensitivity: 'base'});

  // compares 2 arrays lexicographically
  function lex(aParts, bParts) {
    for(var i = 0; i < aParts.length && i < bParts.length; i++) {
      var x = aParts[i];
      var y = bParts[i];
      var comp = collator.compare(x, y);
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
    if (secs < 0)
      return timeUnits(-secs);
    else if (secs >= 1e9)
      return [1e-9, "Gs"];
    else if (secs >= 1e6)
      return [1e-6, "Ms"];
    else if (secs >= 1)
      return [1, "s"];
    else if (secs >= 1e-3)
      return [1e3, "ms"];
    else if (secs >= 1e-6)
      return [1e6, "\u03bcs"];
    else if (secs >= 1e-9)
      return [1e9, "ns"];
    else if (secs >= 1e-12)
      return [1e12, "ps"];
    return [1, "s"];
  }

  function rawUnits(value) {
    if (value >= 1e9)
      return [1e-9, "G"];
    else if (value >= 1e6)
      return [1e-6, "M"];
    else if (value >= 1e3)
      return [1e-3, "k"];
    return [1, ""];
  }

  function formatUnit(raw, unit, precision) {
    var v = precision ? raw.toPrecision(precision) : Math.round(raw);
    var label = String(v) + ' ' + unit;
    return label;
  }

  function formatTime(nsecs, precision) {
    var secs = nsecs * 1e-9;
    var units = timeUnits(secs);
    var scale = units[0];
    return formatUnit(secs * scale, units[1], precision);
  }

  // pure function that produces the 'data' object of the overview chart
  function overviewData(state, reports) {
    var order = state.order;
    var sorter = order === 'report-index' ? reportSort
               : order === 'lex'          ? lexicalSort
               : order === 'colex'        ? reverseLexicalSort
               : order === 'duration'     ? durationSort
               : order === 'rev-duration' ? reverseDurationSort
               : reportSort;
    var sortedReports = reports.filter(function(report) {
      return !state.hidden[report.groupNumber];
    }).slice().sort(sorter);
    var data = sortedReports.map(function(report) {
      return report.time.estPoint;
    });
    var labels = sortedReports.map(function(report) {
      return report.reportName;
    });
    var upperBound = function(report) {
      return report.time.estUpper;
    };
    var errorBars = sortedReports.map(function(report) {
      var est = report.time.estPoint;
      return {
        minus: est - report.time.estLower,
        plus: report.time.estUpper - est,
        color: errorColors[report.groupNumber % errorColors.length]
      };
    });
    var top = sortedReports.map(upperBound).reduce(function(a, b) {
      return Math.max(a, b);
    }, 0);
    var scale = top;
    if(state.activeReport !== null) {
      reports.forEach(function(report) {
        if(report.reportNumber === state.activeReport) {
          scale = upperBound(report);
        }
      });
    }

    return {
      labels: labels,
      top: top,
      max: scale * 1.1,
      reports: sortedReports,
      datasets: [{
        borderWidth: 1,
        backgroundColor: sortedReports.map(function(report) {
          var active = report.reportNumber === state.activeReport;
          var alpha = active ? 'ff' : 'a0';
          var color = colors[report.groupNumber % colors.length] + alpha;
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
      }]
    };
  }

  function inside(box, point) {
    return (point.x >= box.left && point.x <= box.right && point.y >= box.top &&
      point.y <= box.bottom);
  }

  function overviewHover(event, elems) {
    var chart = this;
    var xAxis = chart.scales[chart.options.scales.xAxes[0].id];
    var yAxis = chart.scales[chart.options.scales.yAxes[0].id];
    var point = Chart.helpers.getRelativePosition(event, chart);
    var over =
      (inside(xAxis, point) || inside(yAxis, point) || elems.length > 0);
    if (over) {
      chart.canvas.style.cursor = "pointer";
    } else {
      chart.canvas.style.cursor = "default";
    }
  }

  // Re-renders the overview after clicking/sorting
  function renderOverview(state, reports, chart) {
    var data = overviewData(state, reports);
    var xaxis = chart.options.scales.xAxes[0];
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
    return function(event, elems) {
      var chart = this;
      var xAxis = chart.scales[chart.options.scales.xAxes[0].id];
      var yAxis = chart.scales[chart.options.scales.yAxes[0].id];
      var point = Chart.helpers.getRelativePosition(event, chart);
      var sorted = overviewData(state, reports).reports;

      function activateBar(index) {
        // Trying to activate active bar disables instead
        if (sorted[index].reportNumber === state.activeReport) {
          state.activeReport = null;
        } else {
          state.activeReport = sorted[index].reportNumber;
        }
      }

      if (inside(xAxis, point)) {
        state.activeReport = null;
        state.logaxis = !state.logaxis;
        renderOverview(state, reports, chart);
      } else if (inside(yAxis, point)) {
        var index = yAxis.getValueForPixel(point.y);
        activateBar(index);
        renderOverview(state, reports, chart);
      } else if (elems.length > 0) {
        var elem = elems[0];
        var index = elem._index;
        activateBar(index);
        state.logaxis = false;
        renderOverview(state, reports, chart);
      } else if(inside(chart.chartArea, point)) {
        state.activeReport = null;
        renderOverview(state, reports, chart);
      }
    };
  }

  // listener for sort drop-down
  function overviewSort(state, reports, chart) {
    return function(event) {
      state.order = event.currentTarget.value;
      renderOverview(state, reports, chart);
    };
  }

  // Returns a formatter for the ticks on the X-axis of the overview
  function overviewTick(state) {
    return function(value, index, values) {
      var label = formatTime(value);
      if (state.logaxis) {
        const remain = Math.round(value /
          (Math.pow(10, Math.floor(Chart.helpers.log10(value)))));
        if (index === values.length - 1) {
          // Draw endpoint if we don't span a full order of magnitude
          if (values[index] / values[1] < 10) {
            return label;
          } else {
            return '';
          }
        }
        if (remain === 1) {
          return label;
        }
        return '';
      } else {
        // Don't show the right endpoint
        if (index === values.length - 1) {
          return '';
        }
        return label;
      }
    }
  }

  function mkOverview(reports) {
    var canvas = document.createElement('canvas');

    var state = {
      logaxis: false,
      activeReport: null,
      order: 'index',
      hidden: {},
      legend: false,
    };

    var data = overviewData(state, reports);
    var chart = new Chart(canvas.getContext('2d'), {
      type: 'horizontalBar',
      data: data,
      plugins: [errorBarPlugin],
      options: {
        onHover: overviewHover,
        onClick: overviewClick(state, reports),
        onResize: function(chart, size) {
          if (size.width < 800) {
            chart.options.scales.yAxes[0].ticks.mirror = true;
            chart.options.scales.yAxes[0].ticks.padding = -10;
            chart.options.scales.yAxes[0].ticks.fontColor = '#000';
          } else {
            chart.options.scales.yAxes[0].ticks.fontColor = '#666';
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
          yAxes: [{
            ticks: {
              // make sure we draw the ticks above the error bars
              z: 2,
            }
          }],
          xAxes: [{
            display: true,
            type: axisType(state.logaxis),
            ticks: {
              autoSkip: false,
              min: 0,
              max: data.top * 1.1,
              minRotation: 0,
              maxRotation: 0,
              callback: overviewTick(state),
            }
          }]
        },
        responsive: true,
        maintainAspectRatio: false,
        legend: {
          display: state.legend,
          position: 'right',
          onLeave: function() {
            chart.canvas.style.cursor = 'default';
          },
          onHover: function() {
            chart.canvas.style.cursor = 'pointer';
          },
          onClick: function(_event, item) {
            // toggle hidden
            state.hidden[item.groupNumber] = !state.hidden[item.groupNumber];
            renderOverview(state, reports, chart);
          },
          labels: {
            boxWidth: 12,
            generateLabels: function() {
              var groups = [];
              var groupNames = [];
              reports.forEach(function(report) {
                var index = groups.indexOf(report.groupNumber);
                if (index === -1) {
                  groups.push(report.groupNumber);
                  var groupName = report.groups.slice(0,report.groups.length-1).join(' / ');
                  groupNames.push(groupName);
                }
              });
              return groups.map(function(groupNumber, index) {
                var color = colors[groupNumber % colors.length];
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
          position: 'cursor',
          callbacks: {
            label: function(item) {
              return formatTime(item.xLabel, 3);
            },
          },
        },
        title: {
          display: false,
          text: 'Chart.js Horizontal Bar Chart'
        }
      }
    });
    document.getElementById('sort-overview')
      .addEventListener('change', overviewSort(state, reports, chart));
    var toggle = document.getElementById('legend-toggle');
    toggle.addEventListener('mouseup', function () {
      state.legend = !state.legend;
      if(state.legend) {
        toggle.classList.add('right');
      } else {
        toggle.classList.remove('right');
      }
      renderOverview(state, reports, chart);
    })
    return canvas;
  }

  function mkKDE(report) {
    var canvas = document.createElement('canvas');
    var reportKDE = report.cycles;
    var mean = reportKDE.estPoint;
    var mu = reportKDE.logMean;
    var sigma = Math.sqrt(reportKDE.logVar);
    var units = rawUnits(mean);
    var scale = units[0];

    var numPoints = 100;
    var threshold = 3;
    var minVal = Math.exp(mu - threshold * sigma);
    var maxVal = Math.exp(mu + threshold * sigma);
    var step = (maxVal - minVal) / (numPoints - 1);

    let data = [];
    for (let i = 0; i < numPoints; i++) {
      const x = minVal + i * step;
      const y = Math.exp(-0.5 * Math.pow((Math.log(x) - mu) / sigma, 2));
      data.push({ x: x * scale, y: y });
    }

    var chart = new Chart(canvas.getContext('2d'), {
      type: 'line',
      data: {
        datasets: [{
          label: 'KDE',
          borderColor: colors[0],
          borderWidth: 2,
          backgroundColor: '#00000000',
          data: data,
          hoverBorderWidth: 1,
          pointHitRadius: 8,
        },
          {
            label: 'mean'
          }
        ],
      },
      plugins: [{
        afterDraw: function(chart) {
          var ctx = chart.ctx;
          var area = chart.chartArea;
          var axis = chart.scales[chart.options.scales.xAxes[0].id];
          var value = axis.getPixelForValue(mean * scale);
          ctx.save();
          ctx.strokeStyle = colors[1];
          ctx.lineWidth = 2;
          ctx.beginPath();
          ctx.moveTo(value, area.top);
          ctx.lineTo(value, area.bottom);
          ctx.stroke();
          ctx.restore();
        },
      }],
      options: {
        title: {
          display: true,
          text: report.groups.join(' / ') + ' — time densities',
        },
        elements: {
          point: {
            radius: 0,
            hitRadius: 0
          }
        },
        scales: {
          xAxes: [{
            display: true,
            type: 'linear',
            scaleLabel: {
              display: true,
              labelString: reportKDE.unit + 's',
            },
            ticks: {
              min: minVal * scale,
              max: maxVal * scale,
              callback: function(value, index, values) {
                // Don't show endpoints
                if (index === 0 || index === values.length - 1) {
                  return '';
                }
                var str = String(value) + ' ' + units[1];
                return str;
              },
            }
          }],
          yAxes: [{
            display: false,
            type: 'linear',
          }]
        },
        responsive: true,
        legend: {
          display: false,
          position: 'right',
        },
        tooltips: {
          mode: 'nearest',
          callbacks: {
            title: function() {
              return '';
            },
            label: function(
              item) {
              return formatUnit(item.xLabel, units[1], 3);
            },
          },
        },
        hover: {
          intersect: false
        },
      }
    });
    return canvas;
  }

  function mkScatter(report) {

    // collect the measured value for a given regression
    function getMeasured(key) {
      var ix = report.reportKeys.indexOf(key);
      return report.reportMeasured.map(function(x) {
        return x[ix];
      });
    }

    var canvas = document.createElement('canvas');
    var times = getMeasured("time");
    var iters = getMeasured("iters");
    var lastIter = iters[iters.length - 1];
    var olsTime = report.reportAnalysis.anRegress[0].regCoeffs.iters;
    var dataPoints = times.map(function(time, i) {
      return {
        x: iters[i],
        y: time
      }
    });
    var formatter = iterFormatter();
    var chart = new Chart(canvas.getContext('2d'), {
      type: 'scatter',
      data: {
        datasets: [{
          data: dataPoints,
          label: 'scatter',
          borderWidth: 2,
          pointHitRadius: 8,
          borderColor: colors[1],
          backgroundColor: '#fff',
        },
          {
            data: [
              {x: 0, y: 0 },
              { x: lastIter, y: olsTime.estPoint * lastIter }
            ],
            label: 'regression',
            type: 'line',
            backgroundColor: "#00000000",
            borderColor: colors[0],
            pointRadius: 0,
          },
          {
            data: [{
              x: 0,
              y: 0
            }, {
              x: lastIter,
              y: (olsTime.estPoint - olsTime.estError.confIntLDX) * lastIter,
            }],
            label: 'lower',
            type: 'line',
            fill: 1,
            borderWidth: 0,
            pointRadius: 0,
            borderColor: '#00000000',
            backgroundColor: colors[0] + '33',
          },
          {
            data: [{
              x: 0,
              y: 0
            }, {
              x: lastIter,
              y: (olsTime.estPoint + olsTime.estError.confIntUDX) * lastIter,
            }],
            label: 'upper',
            type: 'line',
            fill: 1,
            borderWidth: 0,
            borderColor: '#00000000',
            pointRadius: 0,
            backgroundColor: colors[0] + '33',
          },
        ],
      },
      options: {
        title: {
          display: true,
          text: report.groups.join(' / ') + ' — time per iteration',
        },
        scales: {
          yAxes: [{
            display: true,
            type: 'linear',
            scaleLabel: {
              display: false,
              labelString: 'Time'
            },
            ticks: {
              callback: function(value, index, values) {
                return formatTime(value);
              },
            }
          }],
          xAxes: [{
            display: true,
            type: 'linear',
            scaleLabel: {
              display: false,
              labelString: 'Iterations'
            },
            ticks: {
              callback: formatter,
              max: lastIter,
            }
          }],
        },
        legend: {
          display: false,
        },
        tooltips: {
          callbacks: {
            label: function(ttitem, ttdata) {
              var iters = ttitem.xLabel;
              var duration = ttitem.yLabel;
              return formatTime(duration, 3) + ' / ' +
                iters.toLocaleString() + ' iters';
            },
          },
        },
      }
    });
    return canvas;
  }

  // Create an HTML Element with attributes and child nodes
  function elem(tag, props, children) {
    var node = document.createElement(tag);
    if (children) {
      children.forEach(function(child) {
        if (typeof child === 'string') {
          var txt = document.createTextNode(child);
          node.appendChild(txt);
        } else {
          node.appendChild(child);
        }
      });
    }
    Object.assign(node, props);
    return node;
  }

  function bounds(kde) {
    return {
      low: kde.estLower,
      mean: kde.estPoint,
      high: kde.estUpper,
    };
  }

  function mkTable(report) {
    var timep4 = function(t) {
      return formatTime(t, 3)
    };
    var cyclesp4 = function(c) {
      return formatUnit(c, report.cycles.unit + 's', 3);
    }
    var rows = [
      Object.assign({
        label: 'Execution cycles',
        formatter: cyclesp4
      },
        bounds(report.cycles)),
      Object.assign({
        label: 'Execution time',
        formatter: timep4
      },
        bounds(report.time)),
    ];
    return elem('table', {
      className: 'analysis'
    }, [
      elem('thead', {}, [
        elem('tr', {}, [
          elem('th'),
          elem('th', {
            className: 'low',
            title: '1-sigma lower confidence bound',
          }, ['lower bound']),
          elem('th', {}, ['estimate']),
          elem('th', {
            className: 'high',
            title: '1-sigma upper confidence bound',
          }, ['upper bound']),
        ])
      ]),
      elem('tbody', {}, rows.map(function(row) {
        return elem('tr', {}, [
          elem('td', {}, [row.label]),
          elem('td', {className: 'low'}, [row.formatter(row.low, 4)]),
          elem('td', {}, [row.formatter(row.mean)]),
          elem('td', {className: 'high'}, [row.formatter(row.high, 4)]),
        ]);
      }))
    ]);
  }

  function slugify(groups) {
    return groups.join('_').toLowerCase()
      .replace(/\s+/g, '_')           // Replace spaces with _
      .replace(/[^\w\-]+/g, '')       // Remove all non-word chars
  }

  document.addEventListener('DOMContentLoaded', function() {
    var rawJSON = document.getElementById('report-data').text;
    var reportData = processReports(JSON.parse(rawJSON));

    var overview = document.getElementById('overview-charts');
    var reports = document.getElementById('reports');
    var overviewLineHeight = 16 * 1.25;

    Object.entries(reportData).forEach(([testName, testData], testNumber) => {
      var tid = slugify([testName]);
      var testOverview = elem('div', { id: tid }, [
        elem('h2', {}, [elem('a', { href: '#' + tid }, [testName])])
      ]);
      overview.appendChild(testOverview);

      Object.entries(testData).forEach(([reportName, report]) => {
        var height = overviewLineHeight * report.numVersions + 36;
        testOverview.appendChild(elem('details', { class: 'group-summary', open }, [
          elem('summary', {}, [reportName]),
          elem('p', { style: 'height: ' + String(height) + 'px' }, [
            mkOverview(Object.values(report.functions).flatMap(f => Object.values(f.versions)))
          ])
        ]));

        var rid = slugify([testName, reportName]);
        var toggleReport = elem('button', {}, ['Expand All']);
        var reportDiv = elem('div', { id: rid }, [
          elem('h1', {}, [elem('a', { href: '#' + rid }, [[testName, reportName].join(' / ')])]),
          toggleReport
        ]);
        reports.appendChild(reportDiv);

        Object.entries(report.functions).forEach(([funcName, func]) => {
          var funcDiv = elem('div', {}, []);
          var details = elem('details', { className: 'report-details' }, [
            elem('summary', {}, [funcName]),
            elem('p', {}, [funcDiv]),
          ]);
          reportDiv.appendChild(details)

          Object.values(func.versions).forEach(version => {
            var id = slugify([testName, reportName, version.reportName]);
            var kde = elem('div', { className: 'kde' }, []);
            funcDiv.appendChild(elem('h3', { id: id }, [
              elem('a', { href: '#' + id }, [version.reportName])]));
            funcDiv.appendChild(kde);
            funcDiv.appendChild(mkTable(version));

            /* Load KDE lazily on demand */
            details.addEventListener('toggle', () => {
              if (details.open && kde.childElementCount === 0)
                kde.appendChild(mkKDE(version));
            });
          });
        });

        const reportDetails = reportDiv.querySelectorAll('details');
        toggleReport.state = false;
        toggleReport.addEventListener('click', () => {
          toggleReport.state = !toggleReport.state;
          toggleReport.textContent = toggleReport.state ? 'Collapse All' : 'Expand All';
          reportDetails.forEach(d => d.open = toggleReport.state);
        });
      });
    });

    const overviewDetails = overview.querySelectorAll('details');
    var toggleOverview = document.getElementById('toggleOverview');
    toggleOverview.state = true;
    toggleOverview.addEventListener('click', () => {
      toggleOverview.state = !toggleOverview.state;
      toggleOverview.textContent = toggleOverview.state ? 'Collapse All' : 'Expand All';
      overviewDetails.forEach(d => d.open = toggleOverview.state);
    });

  }, false);
})();
