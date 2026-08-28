#!/usr/bin/env python3
"""Tests for the spec-flow tooling itself.

scripts/gate.py is the thing every other check is measured by, and it had no
tests of its own (T-053). That mattered concretely: compare_baseline() used to
*skip* any metric the parser had not produced, so a doctest summary whose
wording changed yielded an empty metric set and a confident "BASELINE: MATCH"
that had verified nothing. A gate that fails open is worse than no gate,
because it produces evidence that looks like a pass.

These cover the pure functions only - parsing and comparison. They do not
invoke cmake, the compiler or git, so they run in well under a second and are
safe to run as the first step of the gate itself.

Standard library only, to match gate.py.

    python scripts/test_tooling.py           # verbose
    python -m unittest discover -s scripts   # if you prefer
"""

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import gate  # noqa: E402

# A verbatim doctest tail from this project, kept exactly as the binary emits it
# (including the double space after "test cases:" and the trailing pipe).
REAL_OUTPUT = [
    '[doctest] doctest version is "2.5.0"',
    '[doctest] run with "--help" for options',
    "===============================================================================",
    "C:\\repo\\test_framework\\src\\../test/mvp_gaps_test.h(19):",
    "TEST CASE:  [mvp-gap][planner] planner produces multi-step plans",
    "",
    "C:\\repo\\test_framework\\src\\../test/mvp_gaps_test.h(24): ERROR: CHECK( !steps.empty() ) is NOT correct!",
    "  values: CHECK( false )",
    "",
    "Allowed to fail so marking it as not failed",
    "Allowed to fail so marking it as not failed",
    "Allowed to fail so marking it as not failed",
    "[doctest] test cases:  26 |  26 passed | 0 failed | 0 skipped",
    "[doctest] assertions: 115 | 112 passed | 3 failed |",
    "[doctest] Status: SUCCESS!",
]

BASELINE = {
    "measured": {"commit": "abc1234", "date": "2026-08-28"},
    "build": {"first_party_warnings": 0},
    "test": {
        "test_cases": 26, "test_cases_passed": 26, "test_cases_failed": 0,
        "assertions": 115, "assertions_passed": 112, "assertions_failed": 3,
        "may_fail": 3,
    },
    "format": {"non_conformant": 38, "total": 81, "scope": "all"},
}


class BaselineFixture(unittest.TestCase):
    """Points gate.BASELINE_PATH at a temporary file for the duration."""

    baseline_data: dict | None = BASELINE

    def setUp(self) -> None:
        self._saved = gate.BASELINE_PATH
        self._dir = tempfile.TemporaryDirectory()
        path = Path(self._dir.name) / "baseline.json"
        if self.baseline_data is not None:
            path.write_text(json.dumps(self.baseline_data), encoding="utf-8")
        gate.BASELINE_PATH = path

    def tearDown(self) -> None:
        gate.BASELINE_PATH = self._saved
        self._dir.cleanup()

    def compare(self, metrics, scope="changed", test_ran=True):
        return gate.compare_baseline(metrics, scope, test_ran)

    def assertDrift(self, result, *fragments):
        lines, drifted = result
        self.assertTrue(drifted, f"expected drift, got: {lines}")
        joined = " ".join(lines)
        self.assertIn("BASELINE: DRIFT", joined)
        for f in fragments:
            self.assertIn(f, joined)

    def assertClean(self, result, expect="BASELINE: MATCH"):
        lines, drifted = result
        self.assertFalse(drifted, f"expected no drift, got: {lines}")
        self.assertIn(expect, " ".join(lines))


class TestParseCounts(unittest.TestCase):
    def test_test_cases_line(self):
        line = "[doctest] test cases:  26 |  26 passed | 0 failed | 0 skipped"
        self.assertEqual(
            gate.parse_counts(line, "test cases:"),
            {"total": 26, "passed": 26, "failed": 0, "skipped": 0},
        )

    def test_assertions_line_with_trailing_pipe(self):
        line = "[doctest] assertions: 115 | 112 passed | 3 failed |"
        self.assertEqual(
            gate.parse_counts(line, "assertions:"),
            {"total": 115, "passed": 112, "failed": 3},
        )

    def test_absent_label_returns_empty(self):
        self.assertEqual(gate.parse_counts("[doctest] Status: SUCCESS!", "test cases:"), {})

    def test_non_numeric_fields_are_ignored_not_crashed(self):
        self.assertEqual(gate.parse_counts("test cases: many | lots passed", "test cases:"), {})

    def test_empty_line(self):
        self.assertEqual(gate.parse_counts("", "test cases:"), {})


class TestParseDoctest(unittest.TestCase):
    def test_real_output(self):
        m = gate.parse_doctest(REAL_OUTPUT)
        self.assertEqual(m["test_cases"], 26)
        self.assertEqual(m["test_cases_passed"], 26)
        self.assertEqual(m["test_cases_failed"], 0)
        self.assertEqual(m["assertions"], 115)
        self.assertEqual(m["assertions_passed"], 112)
        self.assertEqual(m["assertions_failed"], 3)

    def test_may_fail_counts_marker_lines_not_doctest_lines(self):
        # The marker is emitted by doctest without a [doctest] prefix, so it is
        # counted by scanning all output rather than only summary lines.
        self.assertEqual(gate.parse_doctest(REAL_OUTPUT)["may_fail"], 3)

    def test_no_may_fail_markers(self):
        self.assertEqual(gate.parse_doctest(["[doctest] test cases: 5 | 5 passed"])["may_fail"], 0)

    def test_garbage_yields_no_counts(self):
        m = gate.parse_doctest(["total nonsense", "not doctest at all"])
        self.assertNotIn("test_cases", m)
        self.assertNotIn("assertions", m)


class TestCompareBaselineHappyPath(BaselineFixture):
    def test_exact_match(self):
        self.assertClean(self.compare(gate.parse_doctest(REAL_OUTPUT)))


class TestCompareBaselineFailsClosed(BaselineFixture):
    """The T-053 regression guards. Unreadable metrics must never read as MATCH."""

    def test_empty_metrics_when_test_ran_is_drift(self):
        self.assertDrift(self.compare({}), "could not parse")

    def test_renamed_doctest_summary_is_drift(self):
        # Simulates doctest changing its wording: may_fail is still counted by a
        # different mechanism, so the metric set is partial rather than empty.
        m = gate.parse_doctest(
            ["[doctest] testcase totals: 26"] + ["Allowed to fail so marking it as not failed"] * 3
        )
        self.assertEqual(m, {"may_fail": 3}, "fixture assumption changed")
        self.assertDrift(self.compare(m), "could not parse", "test_cases")

    def test_skipped_test_step_is_not_reported_as_parse_failure(self):
        # build failed -> test never ran. The build step already failed the gate;
        # blaming the parser here would be a misleading second failure.
        self.assertClean(self.compare({}, test_ran=False))


class TestCompareBaselineDrift(BaselineFixture):
    def test_may_fail_increase(self):
        m = dict(gate.parse_doctest(REAL_OUTPUT), may_fail=4)
        self.assertDrift(self.compare(m), "may_fail 4, expected 3")

    def test_may_fail_decrease_is_still_drift(self):
        # A drop can mean a gap was closed, or that someone deleted the marker.
        # The output cannot distinguish them, so a human must look.
        m = dict(gate.parse_doctest(REAL_OUTPUT), may_fail=2)
        self.assertDrift(self.compare(m), "may_fail 2, expected 3")

    def test_test_cases_dropped(self):
        m = dict(gate.parse_doctest(REAL_OUTPUT), test_cases=25)
        self.assertDrift(self.compare(m), "test_cases 25", "dropped")

    def test_assertions_dropped(self):
        m = dict(gate.parse_doctest(REAL_OUTPUT), assertions=100)
        self.assertDrift(self.compare(m), "assertions 100", "dropped")

    def test_failing_test_case(self):
        m = dict(gate.parse_doctest(REAL_OUTPUT), test_cases_failed=2)
        self.assertDrift(self.compare(m), "test_cases_failed 2")

    def test_new_first_party_warning(self):
        m = dict(gate.parse_doctest(REAL_OUTPUT), first_party_warnings=1)
        self.assertDrift(self.compare(m), "first_party_warnings 1, expected 0")

    def test_format_debt_growth_on_scope_all(self):
        m = dict(gate.parse_doctest(REAL_OUTPUT), non_conformant=40)
        self.assertDrift(self.compare(m, scope="all"), "format debt 40", "grew")


class TestCompareBaselineAhead(BaselineFixture):
    """Improvement must be reported, never punished."""

    def test_more_test_cases_is_ahead_not_drift(self):
        m = dict(gate.parse_doctest(REAL_OUTPUT), test_cases=30)
        self.assertClean(self.compare(m), "BASELINE: AHEAD")

    def test_ahead_names_the_old_value(self):
        m = dict(gate.parse_doctest(REAL_OUTPUT), test_cases=30)
        self.assertIn("was 26", " ".join(self.compare(m)[0]))

    def test_less_format_debt_is_ahead(self):
        m = dict(gate.parse_doctest(REAL_OUTPUT), non_conformant=10)
        self.assertClean(self.compare(m, scope="all"), "BASELINE: AHEAD")

    def test_drift_wins_over_ahead(self):
        # Growing coverage does not excuse a may_fail change.
        m = dict(gate.parse_doctest(REAL_OUTPUT), test_cases=30, may_fail=1)
        self.assertDrift(self.compare(m), "may_fail 1")


class TestFormatScoping(BaselineFixture):
    def test_format_debt_ignored_outside_scope_all(self):
        # A changed-file run legitimately sees fewer files; comparing that to a
        # whole-tree baseline would fail on every ordinary run.
        m = dict(gate.parse_doctest(REAL_OUTPUT), non_conformant=1)
        self.assertClean(self.compare(m, scope="changed"))


class TestMissingBaseline(BaselineFixture):
    baseline_data = None

    def test_absent_file_reports_none_and_does_not_fail(self):
        lines, drifted = self.compare(gate.parse_doctest(REAL_OUTPUT))
        self.assertFalse(drifted)
        self.assertIn("BASELINE: NONE", " ".join(lines))


class TestCorruptBaseline(BaselineFixture):
    def setUp(self):
        super().setUp()
        gate.BASELINE_PATH.write_text("{ not json", encoding="utf-8")

    def test_unparseable_file_does_not_crash_the_gate(self):
        lines, drifted = self.compare(gate.parse_doctest(REAL_OUTPUT))
        self.assertFalse(drifted)
        self.assertIn("BASELINE: NONE", " ".join(lines))


class TestStepVerdict(unittest.TestCase):
    def test_verdicts(self):
        self.assertEqual(gate.Step("build", "", 0).verdict, "PASS")
        self.assertEqual(gate.Step("build", "", 1).verdict, "FAIL")
        self.assertEqual(gate.Step("build", "", gate.SKIPPED).verdict, "SKIP")


if __name__ == "__main__":
    unittest.main(verbosity=2)
