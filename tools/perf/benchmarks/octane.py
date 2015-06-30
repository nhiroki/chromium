# Copyright 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs Octane 2.0 javascript benchmark.

Octane 2.0 is a modern benchmark that measures a JavaScript engine's performance
by running a suite of tests representative of today's complex and demanding web
applications. Octane's goal is to measure the performance of JavaScript code
found in large, real-world web applications.
Octane 2.0 consists of 17 tests, four more than Octane v1.
"""

import os

from core import perf_benchmark

from telemetry import page as page_module
from telemetry.page import page_set
from telemetry.page import page_test
from telemetry.util import statistics
from telemetry.value import scalar

from metrics import power

_GB = 1024 * 1024 * 1024

DESCRIPTIONS = {
  'CodeLoad':
      'Measures how quickly a JavaScript engine can start executing code '
      'after loading a large JavaScript program, social widget being a common '
      'example. The source for test is derived from open source libraries '
      '(Closure, jQuery) (1,530 lines).',
  'Crypto':
      'Encryption and decryption benchmark based on code by Tom Wu '
      '(1698 lines).',
  'DeltaBlue':
      'One-way constraint solver, originally written in Smalltalk by John '
      'Maloney and Mario Wolczko (880 lines).',
  'EarleyBoyer':
      'Classic Scheme benchmarks, translated to JavaScript by Florian '
      'Loitsch\'s Scheme2Js compiler (4684 lines).',
  'Gameboy':
      'Emulate the portable console\'s architecture and runs a demanding 3D '
      'simulation, all in JavaScript (11,097 lines).',
  'Mandreel':
      'Runs the 3D Bullet Physics Engine ported from C++ to JavaScript via '
      'Mandreel (277,377 lines).',
  'NavierStokes':
      '2D NavierStokes equations solver, heavily manipulates double precision '
      'arrays. Based on Oliver Hunt\'s code (387 lines).',
  'PdfJS':
      'Mozilla\'s PDF Reader implemented in JavaScript. It measures decoding '
      'and interpretation time (33,056 lines).',
  'RayTrace':
      'Ray tracer benchmark based on code by Adam Burmister (904 lines).',
  'RegExp':
      'Regular expression benchmark generated by extracting regular '
      'expression operations from 50 of the most popular web pages '
      '(1761 lines).',
  'Richards':
      'OS kernel simulation benchmark, originally written in BCPL by Martin '
      'Richards (539 lines).',
  'Splay':
      'Data manipulation benchmark that deals with splay trees and exercises '
      'the automatic memory management subsystem (394 lines).',
}


class _OctaneMeasurement(page_test.PageTest):
  def __init__(self):
    super(_OctaneMeasurement, self).__init__()
    self._power_metric = None

  def CustomizeBrowserOptions(self, options):
    power.PowerMetric.CustomizeBrowserOptions(options)

  def WillStartBrowser(self, platform):
    self._power_metric = power.PowerMetric(platform)

  def WillNavigateToPage(self, page, tab):
    memory_stats = tab.browser.memory_stats
    if ('SystemTotalPhysicalMemory' in memory_stats and
        memory_stats['SystemTotalPhysicalMemory'] < 1 * _GB):
      skipBenchmarks = '"zlib"'
    else:
      skipBenchmarks = ''
    page.script_to_evaluate_on_commit = """
        var __results = [];
        var __real_log = window.console.log;
        window.console.log = function(msg) {
          __results.push(msg);
          __real_log.apply(this, [msg]);
        }
        skipBenchmarks = [%s]
        """ % (skipBenchmarks)

  def DidNavigateToPage(self, page, tab):
    self._power_metric.Start(page, tab)

  def ValidateAndMeasurePage(self, page, tab, results):
    tab.WaitForJavaScriptExpression('window.completed', 10)
    tab.WaitForJavaScriptExpression(
        '!document.getElementById("progress-bar-container")', 1200)

    self._power_metric.Stop(page, tab)
    self._power_metric.AddResults(tab, results)

    results_log = tab.EvaluateJavaScript('__results')
    all_scores = []
    for output in results_log:
      # Split the results into score and test name.
      # results log e.g., "Richards: 18343"
      score_and_name = output.split(': ', 2)
      assert len(score_and_name) == 2, \
        'Unexpected result format "%s"' % score_and_name
      if 'Skipped' not in score_and_name[1]:
        name = score_and_name[0]
        score = int(score_and_name[1])
        results.AddValue(scalar.ScalarValue(
            results.current_page, name, 'score', score, important=False,
            description=DESCRIPTIONS.get(name)))

        # Collect all test scores to compute geometric mean.
        all_scores.append(score)
    total = statistics.GeometricMean(all_scores)
    results.AddSummaryValue(
        scalar.ScalarValue(None, 'Total.Score', 'score', total,
                           description='Geometric mean of the scores of each '
                                       'individual benchmark in the Octane '
                                       'benchmark collection.'))


class Octane(perf_benchmark.PerfBenchmark):
  """Google's Octane JavaScript benchmark.

  http://octane-benchmark.googlecode.com/svn/latest/index.html
  """
  test = _OctaneMeasurement

  @classmethod
  def Name(cls):
    return 'octane'

  def CreateStorySet(self, options):
    ps = page_set.PageSet(
      archive_data_file='../page_sets/data/octane.json',
      base_dir=os.path.dirname(os.path.abspath(__file__)),
      bucket=page_set.PUBLIC_BUCKET)
    ps.AddUserStory(page_module.Page(
        'http://octane-benchmark.googlecode.com/svn/latest/index.html?auto=1',
        ps, ps.base_dir, make_javascript_deterministic=False))
    return ps
