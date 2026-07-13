# Grok handoff: QtRNetAnalyzer R-Net View column bug

This document is intentionally written for external code review by Grok or another coding agent.
It describes the current local problem and gives strict constraints.

## Repository context

Project: `QtRNetAnalyzer`

Local repo expected by the user:

```text
~/AndroidStudioProjects/QtRNetAnalyzer
```

Related submodule:

```text
external/RNetMsgBroker
```

Related local repo:

```text
~/AndroidStudioProjects/RNetMsgBroker
```

## Hard constraints from the user

Do not use automatic backups.
Do not use online/GitHub state as the source of truth except for push/fetch checks.
Use local repositories under `~/AndroidStudioProjects`.
Do not terminate the user's terminal on errors.
When a script is sourced, end in:

```text
~/AndroidStudioProjects/QtRNetAnalyzer
```

Do not blindly patch. First analyze the actual local files.

## Current user-visible bug

The R-Net View table has a column visibility context menu.
All normal columns can be individually shown/hidden.
The former `Text` column was meant to be removed from the visible model and kept only as tooltip.
However the current UI still has a broken column/data mapping symptom:

```text
Ext, RTR and Timestamp headers are visible, but the cells are empty.
```

The user explicitly wants:

```text
Text column: removed from table entirely
Full text: only as mouseover/tooltip
Visible columns only:
0 Plot
1 #
2 Count
3 ID
4 Name
5 ID parts
6 Fields
7 Data
8 Ext
9 RTR
10 Timestamp
```

## Known intended model truth

`RNetFrameModel::Column` should be exactly:

```cpp
enum Column
{
    ColPlot = 0,
    ColCheck = ColPlot,

    ColRow = 1,
    ColNumber = ColRow,
    ColIndex = ColRow,
    ColNo = ColRow,

    ColCount = 2,
    ColId = 3,
    ColID = ColId,
    ColName = 4,
    ColIdParts = 5,
    ColIDParts = ColIdParts,
    ColFields = 6,
    ColData = 7,
    ColExt = 8,
    ColRTR = 9,
    ColRtr = ColRTR,
    ColTimestamp = 10,
    ColumnCount = 11
};
```

There must be no `ColText` enum and no visible `Text` column.

## Required tooltip behavior

For any visible R-Net table cell:

```cpp
Qt::ToolTipRole -> frame->toString()
```

No hidden table column should be needed for tooltips.

## Known dangerous/incorrect past patches

Do not reintroduce these mistakes:

```cpp
frame->canFrame()      // local RNetFrame does not have canFrame(); it behaves as/contains CanFrame fields directly
ColTag                // local model uses ColPlot, not ColTag
setSectionResizeMode(col, 170) // invalid; second argument must be QHeaderView::ResizeMode
```

Correct width pattern:

```cpp
header->setSectionResizeMode(col, QHeaderView::Interactive);
header->resizeSection(col, 170);
```

## Suspected root cause

`rnetframemodel.h` currently looks mostly consistent, but `mainwindow.cpp` has accumulated several competing R-Net header/resize/menu blocks. This can create stale/contradictory assumptions about visible columns and section indexes.

The fix should likely:

1. Reduce R-Net View column setup in `mainwindow.cpp` to one canonical block.
2. Ensure no code references `ColText`.
3. Ensure the context menu iterates `0 .. RNetFrameModel::ColumnCount - 1` only.
4. Ensure `data()` returns DisplayRole, UserRole and ToolTipRole for columns 0..10 only.
5. Ensure Ext/RTR/Timestamp use the current frame fields and not stale text parsing.
6. Ensure `dataChanged()` ranges end at `ColTimestamp`, never `ColText`.

## Expected R-Net table data roles

DisplayRole:

```text
Plot: checkbox only / empty display
#: row index
Count: numeric count as text
ID: hex CAN ID
Name: decoded name
ID parts: decoded id_parts string
Fields: decoded fields string
Data: payload hex
Ext: 1/0 or EXT/empty; choose consistent display
RTR: 1/0 or RTR/empty; choose consistent display
Timestamp: numeric timestamp
```

UserRole sort values:

```text
Plot: check state numeric
#: numeric row
Count: numeric
ID: numeric CAN ID
Name: normalized text
ID parts: normalized text
Fields: normalized text
Data: normalized text
Ext: 0/1
RTR: 0/1
Timestamp: double
```

ToolTipRole:

```text
All visible columns: frame->toString()
```

## Requested output from Grok

Please produce a minimal patch for `QtRNetAnalyzer` only. The patch must not touch `RNetMsgBroker` unless absolutely necessary.

Prefer a small, readable source patch over broad generated code replacement.

The patch should build with:

```bash
cd ~/AndroidStudioProjects/QtRNetAnalyzer
rm -rf build/Desktop_Debug
git submodule sync --recursive
git submodule update --init --recursive
cmake -S . -B build/Desktop_Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Desktop_Debug -j"$(nproc)"
./build/Desktop_Debug/QtRNetAnalyzer -i testdata/candump.txt
```

## Files to inspect first

```text
src/rnetframemodel.h
src/rnetframemodel.cpp
src/mainwindow.cpp
src/rnetframedelegate.cpp
```

Also inspect the generated diagnostic files in `docs/grok_analysis/`.
