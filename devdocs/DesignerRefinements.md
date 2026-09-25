Designer Refinements {#designer_refinements}
============================================
Working notes for the designer refinement effort (branch `rab_designer_refinements`, started September 2026). They record what was found, what was changed, and what still needs to be done. See @ref designer_log for how to capture and read designer logs.

# Status
Done:
* Restructured designer log (see @ref designer_log): iteration prefix, timed and indented scopes, tagged events, per-POI tables, design summary with restart history, concrete strength controller state when a strength change is refused, and detection of a refinement loop that is not making progress.
* Designer log can be enabled in Release builds and its file name and location controlled with environment variables (`PGS_DESIGN_LOG`, `PGS_DESIGN_LOG_FILE`, `PGS_DESIGN_LOG_DETAIL`). Off by default in Release.
* Infinite-loop bug in `RefineDesignForUltimateMoment` marked in the source with `#pragma Reminder` (see below). **Not fixed.**

The logging changes do not change design results. Designer code paths are unchanged except for logging, and code that only exists for logging is skipped when the log is disabled.

To do:
* Fix the infinite-loop bug.
* Collect designer logs from many projects and quantify where design time goes and why the outer loop restarts (see [Plan](#plan)).
* Decide on efficiency changes based on that data (see [Efficiency ideas](#efficiency_ideas)).
* Condense the logging done by `pgsDesigner2::Check()` ("Final Check for Interval ..."). It writes several lines per POI, has no pass/fail, and uses the old format. It runs after a design and was about 25% of the WF66G log.
* Consider whether `PGS_DESIGN_LOG_DETAIL` should instead follow `WBFL::System::Logger::Verbosity()` so there is one verbosity control in the application. `WBFL::System::Logger` was considered as the basis for the designer log and not used. It is a single, static, application-wide log (the EAF log). The design tools and handling checkers already share a `LogContext&`, and the designer log needs scopes and per-iteration prefixes.

# Known bug: endless loop in ultimate moment refinement {#designer_infinite_loop}
Location: `pgsDesigner2::RefineDesignForUltimateMoment`, marked with `#pragma Reminder("BUG - infinite loop ...")`.

When phiMn < Mu at a POI, the designer adds strands. If the capacity with the added strands is *less* than before, it restores the original number of strands and tries to increase f'c by 500 psi (`Bump500`). If that also fails, it sets the outcome to `UltimateMomentCapacity` and calls `AbortDesign()`, but does not return. Execution falls through to `poiIter = vPoi.begin(); continue;` and rescans the POIs with the design unchanged. The same POI fails the same way, forever. The outer design loop never regains control, so the outer iteration limit does not stop it.

Observed in a Designer_x64.log from 2026-09-24 as more than 100 identical cycles of:

~~~
** Ultimate Flexural Capacity Artifact failed at 62.25 ft. Attempt to add strands
Used demand/capacity ratio of 1.00043 to get a new number of strands = 58
Attempt to add strands succeeded NP = 58
New Capacity = 6738.62 k-ft
We added strands and the capacity did not increase - reduce strands back to original and try bumping concrete strength
Bump 500psi ... A higher concrete strength is required for a different design element. Don't update f'c
Failed increasing concrete strength
Attempt to bump concrete strength failed - we're probably toast at this point, but keep trying to add strands
~~~

The log ended in the middle of a cycle. In the new log format this shows up as `[WARN] POI nnn failed before with the identical design state ... may loop indefinitely`.

Likely fix: `return` after `AbortDesign()` (the outcome and abort flag are already set). Also check:
* `poiIter = vPoi.begin(); continue;` skips `vPoi[0]` on every rescan because the loop increment runs after `continue`.
* Why the capacity went *down* when strands were added (the added strands are presumably higher in the section). Adding strands in a different order, or trying the next strand increment, might be a better response than giving up.
* The f'c bump was refused by the concrete strength controller. The new log reports the controller state when that happens.

# Analysis of a sample design (WF66G, 128 ft)
Design run 2026-09-25 with the new log format. Span 1, Girder 1, WF66G, 128 ft segment. Design options: harping, design for minimum concrete strength, minimize harping fill order, design haunch, no shear design, lifting and hauling design, strand slope design. The log file was not committed; the numbers below are taken from it.

## Result
Outcome: Success. f'c = 5.5 ksi, f'ci = 4.7 ksi, Np = 38 (Ns = 26, Nh = 12), Nt = 0, slab offset 11.25 in, harped strands raised to the top limit at the ends (4 in from top). Lifting at 3 ft, hauling overhangs 5.5 ft.

The design is tight. Each parameter is controlled by a real limit:

Parameter | Controlled by | Demand vs. limit | Use
----------|---------------|------------------|----
Np = 38 | ServiceIII bottom tension at midspan (project criteria target 0.0 ksi) | -0.021 vs. 0.0 ksi | ~100%
f'c = 5.5 ksi | Cast deck ServiceI bottom compression near the ends (x = 3 ft and 125 ft) | -2.454 vs. -2.475 ksi | 99%
f'ci = 4.7 ksi | Lifting (release compression alone needs about 4.5 ksi) | -2.872 vs. -3.055 ksi at release | 94%

Other checks (from the spec check that ran after the design): Open to Traffic ServiceI compression 95% (top, midspan), with live load 88%, FatigueI compression 74%.

The end-region bottom compression at deck casting is what sets f'c. The harped strands are already at the highest end location, so with a harped strategy the only way to reduce it further would be to change the strand arrangement (e.g. debonding, which this strategy does not use).

## How efficiently it was reached
5 outer iterations and 46.4 s. A design that converged directly would have taken about 2 iterations (roughly 20 s).

Iteration | Start state | Time | How it ended
----------|-------------|------|-------------
0 | f'c 5.0, f'ci 4.0, Np 0, slab 11.25 | 9.9 s | End-zone design changed concrete strength (hauling f'c 5.0 to 5.1, lifting f'ci 4.8 to 4.9)
1 | f'c 5.1, f'ci 4.9, Np 39, slab 11.75 | 9.0 s | End-zone design changed concrete strength (lifting f'ci 4.9 to 4.6)
2 | f'c 5.1, f'ci 4.6, Np 37, slab 11.0 | 7.4 s | End-zone design changed concrete strength (lifting f'ci 4.6 to 4.7; hauling f'c 5.1 to 5.0 because a lower value also works)
3 | f'c 5.0, f'ci 4.7, Np 38, slab 11.25 | 3.9 s | Allowable stress refinement: Cast deck bottom compression -2.454 > -2.25 ksi, f'c 5.0 to 5.5
4 | f'c 5.5, f'ci 4.7, Np 38, slab 11.25 | 9.5 s | All steps passed

Observations:

1. **Iterations 0 through 2 oscillated.** f'ci went 4.8, 4.9, 4.6, 4.7; f'c 5.1, 5.0; Np 39, 37, 38; slab offset 11.75, 11.0, 11.25. Slab offset, camber, number of strands, and the release strength required for lifting all depend on each other. Every one of these iterations was restarted by end-zone (lifting/hauling) design changing concrete strength by 0.1 to 0.3 ksi.
2. **The requirement that actually sets f'c was found late.** The cast deck compression check is only evaluated in allowable stress refinement, which is reached after end-zone design stops restarting. That happened for the first time in iteration 3. Every earlier f'c change made for hauling (5.0 and 5.1 ksi) was wasted work, because 5.5 ksi overrides all of them.
3. **Slab offset design is the largest cost.** `DesignSlabOffset` took 20.6 s of 46.4 s (44%) in 10 calls, two per outer iteration (inside mid-zone design and in the outer loop). `DesignMidZone` as a whole took 25.7 s.
4. **The last iteration only confirms convergence.** Iteration 4 changed nothing and took 9.5 s.

Time by design step (scopes are nested, so parents include their children):

Step | Total time | Calls
-----|-----------|------
DesignMidZone | 25.7 s | 5
DesignSlabOffset | 20.6 s | 10
DesignEndZone (harping) | 7.6 s | 5
DesignMidZoneInitialStrands | 6.6 s | 9
DesignForLiftingHarping | 5.5 s | 10
RefineDesignForAllowableStress | 3.0 s | 2
DesignForShipping | 2.1 s | 5
RefineDesignForUltimateMoment | 1.9 s | 1

# Efficiency ideas {#efficiency_ideas}
These are candidates only. They change design behavior, and none of them are implemented. They should be evaluated against many designs (see [Plan](#plan)) and the design regression tests.

* **Establish the f'c floor earlier.** Evaluate final-stage allowable stresses, especially end-region bottom compression at deck casting and final service compression, before or during mid-zone design instead of after end-zone design converges. In the sample this would have set f'c = 5.5 ksi in iteration 0 and avoided the hauling-driven f'c changes.
* **Don't restart for changes that can't matter.** Don't restart the outer loop when handling design lowers f'c or f'ci while another check already requires a higher value, or when a change is within one rounding increment of a value already seen.
* **Reduce slab offset design cost.** Avoid computing it twice per iteration, or skip it when the inputs it depends on (strands, f'ci, f'c) haven't changed since the last call.
* **Damp the lifting f'ci oscillation.** The concrete strength controller already has exponential backoff for decreases; check why f'ci still moved 4.9 to 4.6 to 4.7.
* **Skip the confirmation iteration** when the only change in the last iteration was an f'c increase for a stress check that has since passed. Needs care; the final full-POI check and the spec check are safety nets.

# Plan {#plan}
1. Fix the infinite-loop bug so batch runs can't hang.
2. Collect designer logs from many projects in Release builds:
   * Set `PGS_DESIGN_LOG=1` and `PGS_DESIGN_LOG_FILE=<folder>\` so each project gets `{project}_Designer.log`. The regression batch files start many processes in parallel, which is why the file name should include `{project}` (or `{pid}`).
   * `RegressionTest\Design.bat` runs `/TxDT` (design) on every project in `RegressionTest\Current\Designer-TxDT`. `/TestR` runs the full regression suite, which also designs PGSuper girders. These give a command-line path for batch designs today.
   * Command-line design outside the test agent would need some tweaks (e.g. a switch that designs every girder in a project with the project's design options, saves, and exits, without writing test output).
3. Extract metrics from each log with a script: outcome, number of outer iterations, total time, time by scope name, restart reasons and outcome flags by iteration, oscillating values (f'c, f'ci, Np, slab offset), which check set f'c, f'ci and Np, and `[WARN]` lines.
4. Rank the causes of restarts and time across all designs, then pick efficiency changes from the list above (or new ones) based on the data.
5. For any change, compare the design results before and after across the same set of projects and the design regression tests.
