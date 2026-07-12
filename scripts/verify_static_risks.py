#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import sys
from pathlib import Path

rnet = Path(sys.argv[1]).resolve()
qtra = Path(sys.argv[2]).resolve()
errors = []

cmake = (rnet / "CMakeLists.txt").read_text(encoding="utf-8")
for required in ["src/rnetmsgbroker.h", "src/RNetMsgBroker_global.h", "src/canframe.h", "src/rnetframe.h"]:
    if required not in cmake:
        errors.append(f"RNetMsgBroker CMake vermisst {required}")

if not (rnet / "scripts" / "validate_rnet_json.py").exists():
    errors.append("RNetMsgBroker JSON-Validator fehlt")

mwh = (qtra / "src" / "mainwindow.h").read_text(encoding="utf-8")
if "#define QFileDialog" in mwh:
    errors.append("QtRNetAnalyzer: globales QFileDialog-Makro noch vorhanden")

mwc = (qtra / "src" / "mainwindow.cpp").read_text(encoding="utf-8")
if "QTRNET_ENABLE_DANGEROUS_TX" not in mwc:
    errors.append("QtRNetAnalyzer: TX-Build-Guard fehlt")
if "m_sendBtn->setEnabled(open)" in mwc:
    errors.append("QtRNetAnalyzer: Send-Button wird bei open noch automatisch aktiviert")
if "QtraDeferredCsvFileDialog::getSaveFileName" not in mwc:
    errors.append("QtRNetAnalyzer: CSV-Dialog ist nicht explizit DeferredCsvFileDialog")

rnetmodel = (qtra / "src" / "rnetframemodel.cpp").read_text(encoding="utf-8")
if "case ColCount: return QString::number" not in rnetmodel:
    errors.append("QtRNetAnalyzer: Count-Spalte wird nicht als sichtbarer QString geliefert")
if "ColCount, QHeaderView::Fixed" not in mwc and "setColumnWidth(RNetFrameModel::ColCount" not in mwc:
    errors.append("QtRNetAnalyzer: Count-Spaltenbreite wird nicht explizit gesetzt")
if "setItemDelegateForColumn(RNetFrameModel::ColTag" not in mwc:
    errors.append("QtRNetAnalyzer: RNetFrameDelegate ist nicht auf Plot-Spalte begrenzt")

rnetdelegate = (qtra / "src" / "rnetframedelegate.cpp").read_text(encoding="utf-8")
if "index.column() == RNetFrameModel::ColCount" not in rnetdelegate:
    errors.append("QtRNetAnalyzer: Count-Delegate-Fallback fehlt")

if errors:
    for e in errors:
        print("RISIKO-PRÜFUNG FEHLER:", e, file=sys.stderr)
    sys.exit(1)

print("Statische Risiko-Prüfung OK")
