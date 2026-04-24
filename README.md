# RNet Qt6 Analyzer

Eine vollständige Qt6-Widgets-Beispielanwendung für R-Net/CAN-Frames.

## Enthalten

- `CanFrame` Grundtyp
- `RNetFrame`-Hierarchie
- `RNetFrameModel` mit Dekodierung, Aggregation und Tabellenanzeige
- Live-Simulator für Beispiel-Frames
- Qt6/CMake-Projektstruktur

## Build

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
./RNetQt6Analyzer
```

## Architektur

- Neue `CanFrame`s werden per `appendCanFrame()` dekodiert.
- Bereits bekannte Nachrichten werden über `m_rowByKey` zusammengefasst.
- Neue dekodierte `RNetFrame`s werden ans Ende von `m_rows` angehängt.

## Hinweis

Der mitgelieferte `rnetframe.h` aus der Unterhaltung wurde als fachliche Basis übernommen, aber für ein kompilierbares Qt6-Projekt bereinigt und vervollständigt.

## Integration echter Hardware

Für reale CAN-Hardware kann der Simulator durch eine eigene `RNetSource`-Implementierung ersetzt werden, die `frameReceived(const CanFrame&)` emittiert.
