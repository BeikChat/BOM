# BOM Analyzer

Desktop application for loading, visualizing and analyzing BOM (bill of materials) data, plus `profile_analyzer.dll` — a C++ library with a
plain C ABI that is consumed both by the Qt application and by legacy Delphi code.

---

## Contents

| Path | What |
|---|---|
| `components/core/` | **bom_core** — entities (data + business rules) and computational core. Depends on Qt Core only. `tests/` — its unit tests. |
| `components/app/` | **bom_app** (UI, `infrastructure/` CSV/JSON I/O, DLL adapter) + **bom_viewer.exe** (`main.cpp`). `tests/` — its unit tests. |
| `components/profile_analyzer/` | **profile_analyzer.dll** — standalone C++ library, no Qt, C ABI, `__stdcall`. `tests/` — its unit tests. |
| `components/delphi/` | `src/ProfileAnalyzerWrapper.pas` (Delphi unit), `src/ProfileAnalyzerDemo.dpr` (console demo), CMake build of the demo. |
| `cmake/` | `BomTesting.cmake` (`bom_add_test()`). |
| `tests/` | Integration tests. |
| `data/` | samples of .cvs data. |
| `build/` | Output directory for built artifact. |
| `stage/` | Output directory for built and deployed artifact. |

## Build

Requirements: CMake ≥ 3.22, Ninja, compiler with C++23 support, Qt 6.10+. 
Optional: vcpkg, Free Pascal or Delphi.

### Visual Studio

* **Open Folder** (recommended): *File → Open → Folder…*, pick the repository. Visual Studio
  reads `CMakePresets.json`; choose the configure preset in the toolbar:
  * `x64 (Qt from an installation, QTDIR)` — set the environment variable `QTDIR` to the Qt
    kit first, e.g. `C:\Qt\6.9.2\msvc2022_64`;
  * `x64 (Qt from vcpkg)` — needs `VCPKG_ROOT` (a vcpkg checkout or the vcpkg bundled with
    Visual Studio); the first configure builds Qt and takes long.

  Select `bom_viewer.exe` as the startup item and press F5. *Build → Install* fills
  `stage/<preset>`.
* **Solution**: `cmake -S . -B build/vs -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.9.2/msvc2022_64`,
  open `build/vs/BomAnalyzer.sln`. `bom_viewer` is the startup project and opens
  `data/sample_bom.csv` when debugged. The `INSTALL` project fills the install prefix
  (`-DCMAKE_INSTALL_PREFIX=stage/vs`).

### Command line

The presets use Ninja with an external toolset: run them from a *Developer Command Prompt*
(x64 or x86, matching the preset).

```bat
:: x64, Qt from an installation
set QTDIR=C:\Qt\6.9.2\msvc2022_64
cmake --preset x64
cmake --build --preset x64-release
ctest --preset x64-release
build\x64\bin\Release\bom_viewer.exe data\sample_bom.csv

:: self-contained folder stage\x64 (Qt runtime deployed by windeployqt, Delphi sources, DLL SDK)
cmake --build --preset x64-stage

:: x64, Qt built by vcpkg from vcpkg.json (VCPKG_ROOT must be set)
cmake --preset x64-vcpkg
cmake --build --preset x64-vcpkg-release
ctest --preset x64-vcpkg-release
cmake --build --preset x64-vcpkg-stage

:: x86: profile_analyzer.dll + Delphi demo for 32-bit Delphi applications (no Qt needed)
cmake --preset x86-dll
cmake --build --preset x86-dll-stage
ctest --preset x86-dll-release
```

`-DBOM_BUILD_APP=OFF` builds only the DLL, `-DBOM_BUILD_TESTS=OFF` skips tests,
`-DBOM_BUILD_DELPHI=OFF` skips the Delphi demo.

### Delphi part

`components/delphi/src/ProfileAnalyzerWrapper.pas` + `ProfileAnalyzerDemo.dpr` open directly in Delphi
(2009 or newer). Put `profile_analyzer.dll` of the **same bitness** as the target platform
next to the executable.

The CMake build compiles `ProfileAnalyzerDemo.exe` for the bitness of the build (x86 preset →
Win32, x64 preset → Win64) next to `profile_analyzer.dll`, and CTest runs it against the DLL
(`delphi_demo_*` integration tests in `tests/`).

The build of the demo is described in [`components/delphi/CMakeLists.txt`](components/delphi/CMakeLists.txt).
The compiler is the Delphi command-line compiler (`dcc32`/`dcc64`, XE2+) if it is on `PATH`,
otherwise Free Pascal; `-DDELPHI_USE_COMPILER=AUTO|DCC|FPC` forces one, and the paths can be set
with `-DDELPHI_DCC32_EXECUTABLE=...`, `-DDELPHI_DCC64_EXECUTABLE=...`, `-DDELPHI_FPC_EXECUTABLE=...`.
The platform follows the C++ toolchain (Win32/Win64), so Pascal binaries always match the DLL.

Delphi was not available on the development machine, so the unit was verified with
**Free Pascal 3.2.2** (Win32 and Win64) against the x86 and x64 DLLs; the `dcc32`/`dcc64` branch
of the build is untested. FPC compiles with `-Mdelphiunicode`, i.e. with Delphi's lexical rules
from the very first line (for example, no nested comments), so code that FPC would tolerate but
Delphi would reject fails the build as well.

## Features

* **Tree view** (`QTreeView` + custom `QAbstractItemModel`): BOM (building) → positions.
  Columns: length, cost per piece, quantity, position cost (`Cost × Quantity`),
  cost per metre (`Cost / Length_mm × 1000`).
  * BOM rows show aggregates: total cost, total pieces, length-weighted price per metre.
  * Selection (extended, rows) — the status bar shows the number of selected rows.
  * Expand / collapse (toolbar, context menu, keyboard).
  * Context menu: *Analyze profile*, copy (TSV to clipboard), expand/collapse.
  * In-cell controls — custom delegate `BomItemDelegate`: `QDoubleSpinBox` for length
    (suffix “мм”, range 0.01 mm … 1 km) and cost (≥ 0), `QSpinBox` for quantity (1 … 1 000 000).
    The length cell is also displayed with its unit (“6 000 мм”). A loaded value outside the
    editor's range or with more decimals than it shows is never clamped or rounded just by
    opening and closing the editor. Edits are validated
    by the model and immediately recompute all analytics.
  * Filtering by BOM id or profile name and sorting by any column (`QSortFilterProxyModel`,
    recursive filtering).
* **CSV loading**: menu/toolbar, command line argument, or drag & drop.
* **Analytics panel**: grand total, per-BOM table, top-3 profiles, per-profile statistics
  (average price per metre, total length, total cost, min/max price and spread), price deviations.
* **“Сохранить отчёт”** exports a JSON report through a save dialog (atomic write via `QSaveFile`).
* **“Анализ профиля”** calls `profile_analyzer.dll` directly (no Delphi) and shows the result
  in a small dialog, including the total weight of that profile across all loaded BOMs.

## Architecture

```
            ┌─────────────────────────── bom_viewer.exe ──────────────────────────────┐
 UI         │ MainWindow · BomTreeModel · SummaryPanel · dialogs                      │  Qt Widgets
 Infra      │ infrastructure: CsvBomReader → ParseIssue[] · ReportJsonWriter          │
            │ services/ProfileAnalyzerLibrary (QLibrary) ─────────────────────────────┼──► profile_analyzer.dll
            └───────────────┬─────────────────────────────────────────────────────────┘        ▲
                            │ uses                                                            │ LoadLibrary
 Core       ┌───────────────▼──────────── bom_core (static, Qt Core) ─────────────────┐       │
            │ analytics: BomAnalyzer → AnalyticsReport                                │   Delphi modules
 Entities   │ domain:    BomItem · Bom · BomDataset (rules + formulas)                │
            └─────────────────────────────────────────────────────────────────────────┘
```

* **Entities** (`bom::core` domain) hold the CSV fields together with the business rules and
  formulas that belong to them:
  * `BomItem` — `totalCost()` (`Cost * Quantity`), `costPerMeter()` (`Cost / Length_mm * 1000`),
    `totalLengthMm()/M()`, and the validation rules `isValidLength/Cost/Quantity()`, `isValid()`.
    The CSV reader and the tree model (in-cell editing) both use these rules, so a value is
    valid or invalid in the same way everywhere;
  * `Bom` — aggregates of one building: `totalCost()`, `totalQuantity()`, `totalLengthM()`,
    length-weighted `averageCostPerMeter()`;
  * `BomDataset` — the list of BOMs and `addItem(buildingId, item)` (the grouping by BuildingID).

  They stay simple structs (public fields, aggregate initialization) — the rules are checked
  where data enters the program (CSV loading, editing), not by setters.
* **Computational core** — `BomAnalyzer` is stateless: `static AnalyticsReport analyze(const BomDataset&)`;
  totals over all BOMs are summed from the per-BOM results. The size of the top
  (`kTopProfilesCount = 3`) and the price-deviation threshold (`kPriceDeviationThresholdPercent = 10`)
  are constants of the core; the UI takes its captions from them.
  Easy to test and to run in a worker thread if datasets ever get large.
* **Infrastructure** (`bom::infrastructure`, in the application): the CSV reader produces a
  dataset + diagnostics, the JSON writer serializes a report. Neither knows about widgets.
* **UI** only adapts: the tree model exposes the dataset (no caches — aggregates are computed
  on request) and writes edits back; `MainWindow` wires signals and contains no calculations.
* The DLL adapter `ProfileAnalyzerLibrary` (owned by `MainWindow`) loads the DLL on first use
  and turns the C call into a Qt-friendly `ProfileAnalysis` result.
* `bom_app` is a static library separate from the executable, so the model and the
  infrastructure are unit-tested (including `QAbstractItemModelTester`).

## Decisions and interpretations

**CSV error handling — “best effort, nothing silent”.**
An invalid row is skipped and reported (line number + *all* problems of the row at once);
a suspicious but usable row is kept with a warning.
The load fails as a whole only if the file can’t be read, the header lacks a required
column, or no valid row is left — in that case the previously loaded data stays on screen.
Diagnostics are shown in the “Журнал загрузки” dock. Validation rules: non-empty
`BuildingID`/`ProfileName`, `Length_mm > 0`, `Cost ≥ 0`, `Quantity` — integer `> 0`,
exact field count.
The format is the one of the task: `BuildingID,ProfileName,Length_mm,Cost,Quantity`, comma
delimiter, `.` as decimal point. The header line is optional; if present it must list exactly these
columns in this order (otherwise the load fails with a clear message).

**“Average cost per metre” of a profile** is **length-weighted**: `Σ cost / Σ length`.
A plain mean of per-position prices would let a 1 m offcut weigh as much as a 12 m beam.
Min/max price per metre and the spread are reported next to it.

**Top-3 profiles** are ranked by the cost of the profile summed over all its positions in all
BOMs (`Σ Cost × Quantity` per profile name), not by single CSV rows — so one profile never
occupies several places of the top. Ties keep the order of first appearance in the source
(stable sort), so results are deterministic.

**Unique profile names** are compared exactly, as written in the CSV (after trimming and
collapsing whitespace), as the task asks for “each unique profile name”. `IPE200` and `IPE 200`
are therefore two profiles; spotting such spelling variants is left to the source system.

**Editing** in the tree changes only the in-memory data (the source CSV is never modified);
the title shows `*` and the JSON report gets `"editedAfterLoad": true`.

**Additional business metrics** in the report:

| Metric | Why it matters |
|---|---|
| `boms[].shareOfTotalCostPercent`, `summary.mostExpensiveBom` | where the budget goes per building |
| `summary.topProfilesSharePercent`, `profiles[].shareOfTotalCostPercent` | concentration of cost in a few profiles |
| `profiles[].min/maxCostPerMeter`, `priceSpreadPercent` | the same profile bought at different prices |
| `priceDeviations[]` | positions > 10 % off their profile’s average price, with the over-/underpayment in money |

### JSON report

```jsonc
{
  "generatedAt": "2026-11-10T08:00:00Z",
  "generator": "BOM Analyzer 1.0.0",
  "sourceFile": "sample_bom.csv",
  "editedAfterLoad": false,
  "summary": { "bomCount", "positionCount", "uniqueProfileCount", "totalQuantity", "totalLengthM",
               "totalCost", "averageBomCost", "averageCostPerMeter", "mostExpensiveBom",
               "topProfilesSharePercent" },
  "boms":            [ { "buildingId", "positionCount", "totalQuantity", "totalLengthM", "totalCost",
                         "averageCostPerMeter", "shareOfTotalCostPercent" } ],
  "topProfiles":     [ /* the 3 most expensive profiles, same objects as in "profiles" */ ],
  "profiles":        [ { "profileName", "positionCount", "bomCount", "totalQuantity",
                         "totalLengthM", "totalCost", "shareOfTotalCostPercent", "averageCostPerMeter",
                         "minCostPerMeter", "maxCostPerMeter", "priceSpreadPercent" } ],
  "priceDeviations": [ { "buildingId", "profileName", "costPerMeter",
                         "profileAverageCostPerMeter", "deviationPercent", "overpayment" } ]
}
```
Money is rounded to 2 decimals, prices per metre to 4, lengths in metres to 3.
No currency is assumed (the CSV has none).

## profile_analyzer.dll

```c
int __stdcall AnalyzeProfile(const wchar_t* profileName, double* nominalHeight,
                             double* weightPerMeter, wchar_t* profileType);
const wchar_t* __stdcall GetAnalyzeProfileErrorText(int code);
```

| Code | Meaning |
|---|---|
| `0` `PA_OK` | weight from the reference table |
| `-1` `PA_E_INVALID_ARGUMENT` | NULL pointer |
| `-2` `PA_E_EMPTY_NAME` | empty / whitespace name |
| `-3` `PA_E_INVALID_FORMAT` | unparsable name or wrong number of dimensions |
| `-4` `PA_E_UNKNOWN_TYPE` | valid syntax, unsupported family (type and height are still returned) |
| `-5` `PA_E_OUT_OF_RANGE` | dimensions outside the supported range |
| `-100` `PA_E_INTERNAL` | unexpected internal error |

**Parsing.** Type = leading Latin letters (case-insensitive, upper-cased); then an optional
` `/`-`/`_`; then dimensions separated by `*`, `x` or `/`; `.` or `,` as decimal point.
`IPE200`, `hea 160`, `L150*5`, `L100x75x8`, `RHS100x50x4`, `CHS114.3x4` are valid;
`IPE200 S235` (trailing text) is not. Nominal height = first dimension.

**Weight per metre.**
* `IPE`, `HEA`, `HEB`, `UPN` — reference table (EN 10365 nominal masses). A height between two
  table rows is linearly interpolated; outside the table → `-5`.
* `L` (equal/unequal angle), `RHS`, `SHS`, `CHS`, `FL`/`PL`, `RD` — cross-section area from the
  dimensions × 7850 kg/m³ (fillet radii neglected, ≈1–3 % lower than catalogue values).

**ABI decisions.**
* `extern "C"` + `__stdcall` (native for Delphi `stdcall`), names exported through a `.def`
  file so they are undecorated on x86 too (`AnalyzeProfile`, not `_AnalyzeProfile@16`).
* UTF-16 strings — `wchar_t` on Windows is exactly Delphi’s `WideChar`/`string` element.
* The signature has no size for `profileType`, so the contract is a fixed buffer of
  `PA_PROFILE_TYPE_BUFFER_SIZE = 32` wide chars; the DLL guarantees not to write more
  (longer types are rejected as `-3`). Input length is bounded (`PA_PROFILE_NAME_MAX_LENGTH`).
* Outputs are always reset first; no C++ exception crosses the boundary (`catch (...)`).
* The CRT is linked statically: the DLL depends only on `KERNEL32.dll`, so Delphi teams don’t
  need a VC++ redistributable. Safe because no CRT-owned resources cross the ABI.
* Built for both **x86** and **x64**.

**Delphi wrapper.** `ProfileAnalyzerWrapper.pas` keeps the interface from the task
(`TProfileInfo`, `AnalyzeProfile(const string; out TProfileInfo): Boolean`). The DLL is loaded
lazily on the first call from the directory of the module that contains the unit
(`GetModuleName(HInstance)` — correct also when the unit lives in a DLL/BPL; absolute path — no
DLL search-order hijacking), `GetProcAddress` result is validated, and `FreeLibrary` is called in
`finalization`. Loading is guarded by a critical section. Floating-point exceptions are masked
for the duration of the call (`SetExceptionMask`): Delphi unmasks them by default, while
MSVC-compiled code assumes they are masked. Because a `Boolean` loses the reason of a failure,
the unit additionally exposes `ProfileAnalyzerLastErrorCode/Text` (per thread); a DLL that cannot
be loaded is reported as `PA_E_LIBRARY_NOT_LOADED = -1000`.
The Qt application uses the same strategy with `QLibrary` and the same `-1000` code
(`kProfileAnalyzerNotLoaded`), so “DLL missing” is never confused with `PA_E_INTERNAL`.

## Tests

Unit tests are next to the code they test (`components/<name>/tests/`), integration tests —
several components together, with the real DLL and real files — are in `tests/`. All are Qt Test
executables registered in CTest by `bom_add_test()` ([`cmake/BomTesting.cmake`](cmake/BomTesting.cmake)),
except the Delphi ones. `ctest --preset x64-release` runs all of them; the `x86-dll` preset (no Qt)
runs only the Delphi integration tests.

**Unit tests**
* `profile_analyzer/tests/tst_profile_analyzer` — the DLL through its public C ABI: table/formula/interpolated cases,
  every error code, NULL pointers, oversized input, output reset;
* `core/tests/tst_domain` — formulas, validation rules and grouping of the entities (`BomItem`, `Bom`, `BomDataset`);
* `core/tests/tst_bom_analyzer` — report aggregates on hand-computed data, top-3 profiles summed
  per name, length-weighted averages, price deviations;
* `app/tests/tst_csv_bom_reader` — header handling, BOM/CRLF, every validation error,
  the `sample_bom_with_errors.csv` file line by line, duplicates, fatal cases;
* `app/tests/tst_report_json_writer` — JSON structure, file round trip, write failure;
* `app/tests/tst_bom_tree_model` — `QAbstractItemModelTester`, editing, validation; the delegate: editor
  types and ranges, committing an edit, the “мм” unit;
* `app/tests/tst_summary_panel` — the “Сводка” tab: captions and values, empty state, captions taken from
  the core constants (top size);

Sample datasets for the core and application unit tests are in `core/tests/TestData.h`
(CMake target `bom_core_test_data`).

**Integration tests** (`tests/`)
* `delphi_demo_*` — `ProfileAnalyzerWrapper.pas` through the demo against the DLL: valid names,
  conversion into `TProfileInfo`, error code of an unsupported type (also in the `x86-dll` preset);
* `tst_profile_analyzer_library` — the Qt adapter `ProfileAnalyzerLibrary` loads the real DLL
  (QLibrary), result and error text of the DLL, a missing DLL, every profile of `sample_bom.csv`
  is recognized;
* `tst_report_pipeline` — `sample_bom.csv` → `CsvBomReader` → `BomAnalyzer` → `ReportJsonWriter`
  → JSON file, checked against values computed by hand; a file with errors still gives a report.

## Reflection

The task looks like “a tree and a few sums”, but most of the interesting decisions were in the
details that the statement leaves open: what a *valid* CSV row is and what to do with an invalid
one, what “average price per metre” should mean for a business user (weighted vs. plain mean),
and how to design a binary interface that a Delphi developer can call safely without reading
the C++ source.

The hardest part was the C++/Delphi boundary: calling convention and name decoration on x86,
string representation, buffer ownership with a fixed signature that has no buffer size,
exceptions, CRT dependencies and bitness. Each of these is a classic source of crashes that only
show up on the customer’s machine, so they are handled explicitly and covered by ABI-level tests.
The lack of a Delphi installation was compensated by compiling the unit with Free Pascal in Delphi
Unicode mode against the 32-bit DLL.

I enjoyed the task: it is close to real work — a small vertical slice touching data quality,
analytics, UI and legacy interop — and it leaves room for judgement rather than just coding.
