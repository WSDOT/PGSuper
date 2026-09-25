Designer Log {#designer_log}
============================
# Overview
The girder designer (`pgsDesigner2` and the strand, shear, lifting, and hauling design tools in the EngAgent) writes a log that records how a design was reached. The log is meant to be read by a person following the design algorithm and by tools (grep, scripts, LLMs) diagnosing a failed, slow, or non-converging design.

The implementation is in `EngAgent/DesignLog.h` and `EngAgent/DesignLog.cpp`. See @ref designer_refinements for the analysis that motivated the current format and for open issues.

# Enabling the log
The designer log is compiled into all builds and controlled at run time with environment variables. The variables are read when a design starts.

Variable | Values | Description
---------|--------|------------
`PGS_DESIGN_LOG` | `1` or `0` | Enables or disables the designer log. When not set, the log is enabled in Debug builds (`ENABLE_LOGGING`) and disabled in Release builds.
`PGS_DESIGN_LOG_FILE` | path | Name and location of the log file. When not set, the log is `Designer_x64.log` (`Designer.log` for 32-bit builds) in the current working folder.
`PGS_DESIGN_LOG_DETAIL` | `1` or `0` | Also log bulky per-calculation detail (moment capacity details at every POI, debond level construction, relief stress calculations, etc.). Off by default.

`PGS_DESIGN_LOG_FILE` can be a folder (ends with `\` or is an existing folder). In that case the log file is named `{project}_Designer.log`. The following tokens are replaced in the path:

Token | Replaced with
------|--------------
`{project}` | Project file title (file name without extension), or `Untitled`
`{projectdir}` | Folder containing the project file
`{timestamp}` | Date and time the log was opened, `yyyymmdd_hhmmss`
`{pid}` | Process id

Missing folders are created. One log file is written per project (designer instance), so the tokens make it possible to capture a separate log for every project in a batch run. Examples:

~~~
rem Log next to each project file
set PGS_DESIGN_LOG=1
set PGS_DESIGN_LOG_FILE={projectdir}\{project}_Designer.log

rem Collect logs for a regression run in one folder (runs are in parallel, so use {project})
set PGS_DESIGN_LOG=1
set PGS_DESIGN_LOG_FILE=C:\DesignerLogs\
call RegressionTest\Design.bat
~~~

When the log is disabled, `DLOG` statements do not evaluate their arguments and logging-only code blocks are skipped, so the cost is a flag test. Design results are the same whether or not the log is enabled.

# Log format
A log begins with a legend describing the conventions, then the project path and the log file path.

* **Iteration prefix.** Every line inside an outer design iteration starts with `[iNN]`. Lines outside the outer loop start with `[---]`. Search for `[i03]` to see everything in iteration 3.
* **Scopes.** Each design step opens with `>> Name` and closes with `<< Name (1.234 s) -> result`. Lines inside a scope are indented with `|  `. Scopes are exception safe; a cancelled design still closes them and says so.
* **Tags.** Important events are tagged so they can be found with a single search:

Tag | Meaning
----|--------
`[FAIL]` | A check did not pass. Gives what failed, where, and demand vs. limit.
`[ACTION]` | The designer changed the design (strands, f'c, f'ci, debonding, harped offsets, slab offset, handling locations). Gives the old and new value.
`[RESTART]` | The outer design loop is restarting. Gives the reason and the designer outcome flags that were set.
`[ABORT]` | The design gave up. Gives the design step and the outcome code.
`[WARN]` | Unexpected condition or a sign of non-convergence (e.g. the same POI failing with an identical design state).
`[OK]` | A step or check completed successfully.

* **Units** are always stated: x locations in ft measured from the start of the segment, section dimensions in in, stress in ksi, force in kip, moment in kip-ft.
* **Tables.** Allowable stress refinement and ultimate moment refinement log one row per POI with a header row, instead of blocks of lines per POI. Full capacity details are logged automatically at a failing POI.
* **Design summary.** Each girder design ends with a `DESIGN SUMMARY` block: outcome, design options, number of outer iterations, every restart with its reason and outcome flags, and the final design state. This is the first thing to read.

A short example:

~~~
[i03] |  |  |  >> Stress check: Interval 11 (Cast Deck), ServiceI Compression
[i03] |  |  |  |  Stress limit = -2.25 ksi (compression, concrete strength used = 5 ksi)
[i03] |  |  |  |          x (ft)    POI | top: ps      res min   res max | bot: ps      res min   res max | result
[i03] |  |  |  |           3.000    340      -0.477    -0.584    -0.584      -2.554    -2.454    -2.454 | FAIL: compression at bottom exceeds limit
[i03] |  |  |  |          64.000    367       0.847    -1.770    -1.770      -3.817    -1.382    -1.382 | OK
[i03] |  |  |  |  [FAIL] Stress limit exceeded. Controlling stress = -2.45419 ksi at bottom, limit = -2.25 ksi. Need higher f'c
[i03] |  |  |  |  [ACTION] f'c changed from 5 to 5.5 ksi
[i03] |  |  |  << Stress check: Interval 11 (Cast Deck), ServiceI Compression (0.155 s) -> FAIL, concrete strength changed
[i03] |  |  |  [RESTART] Allowable stress refinement changed concrete strength [FcIncreased, ...]
~~~

# Reading a log
Useful searches:

Search | Finds
-------|------
`DESIGN SUMMARY` | Outcome, restart history, and final state of each girder design
`[RESTART]` | Why the outer loop restarted
`[ABORT]` | Why a design gave up
`[WARN]` | Non-convergence symptoms, such as an endless refinement loop
`[FAIL]` | Every failed check with demand and limit
`[ACTION]` | Every change made to the design, in order
`<< Outer design iteration` | Time spent in each outer iteration and how it ended
`not changed (requested` | Concrete strength requests refused by the concrete strength controller, with the controller state that refused them

Elapsed times on the `<<` lines can be summed by scope name to find where design time goes. Scopes are nested, so parents include the time of their children.

# Writing to the log
Designer code uses the macros in `DesignLog.h`, not the `LOG` macro from `IFace/Tools.h`. `LOG` belongs to the other agent logs and only exists in builds with `ENABLE_LOGGING`.

Macro | Use
------|----
`DLOG(x)` | Write a line. `x` is a stream expression, e.g. `DLOG(_T("Np = ") << np)`.
`LOG_FAIL`, `LOG_ACTION`, `LOG_RESTART`, `LOG_ABORT`, `LOG_WARN`, `LOG_OK` | Tagged lines
`LOG_DETAIL(x)` | Bulky detail, only written when `PGS_DESIGN_LOG_DETAIL=1`
`DESIGN_LOG_SCOPE(x)` | Open a timed scope that lasts until the end of the C++ block (one per block)
`DESIGN_LOG_SCOPE_RESULT(x)` | Set the text on the closing line of the innermost scope
`DESIGN_LOG_ONLY(x)` | Code that only exists for logging (e.g. a stream built over a loop)

Guidelines:

* State units and use the `pgsDesignLog::ksi()`, `ft()`, `in()`, `kip()`, and `kipft()` helpers.
* Log a failure with the demand, the limit, and the location in one line.
* Log a change with the old value and the new value. Don't tag a setter call as `[ACTION]` when the value didn't change.
* Prefer one row per POI in a table over several lines per POI. Put per-calculation detail in `LOG_DETAIL`.
* Code that only computes values for the log (extra broker queries, string building in loops) must be inside `if (pgsDesignLog::IsEnabled())` so a disabled log costs nothing.

The design tools and handling checkers take a `WBFL::Debug::LogContext&` (`DESIGN_SHARED_LOGFILE`), which is the same type in all builds. Callers outside the designer pass `AGENT_LOGGER_FOR_DESIGN_TOOLS`, which is the caller's own log in Debug builds and a log that discards everything in Release builds.
