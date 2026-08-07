# Features


## Version 0 (Standard Version) - The Format


### 1. Was ist ein Samfile?

Ein Samfile ist die Konfigurationsdatei eines **Task-Runners** — vergleichbar
mit einem `Makefile` (Make), einem `Taskfile.yml` (Task) oder einem
`justfile` (Just). Es definiert benannte **Tasks**, die aus einer Liste von
Befehlen bestehen und über die CLI (vermutlich `sam <taskname>`) ausgeführt
werden.

Grundidee: Statt Build-, Deploy- und Hilfsschritte immer wieder von Hand
einzutippen, schreibst du sie einmal als Task auf und rufst sie über einen
kurzen Namen auf.


### 2. Grundbausteine

Ein Samfile besteht aus drei Arten von Zeilen:

| Element | Beispiel | Bedeutung |
|---|---|---|
| Makro-Definition | `#define SEG seg` | Textersetzung, ähnlich C-Präprozessor |
| Kommentar | `# Build the it with CMake` | wird ignoriert |
| Task-Definition | `build:` | definiert einen neuen, benannten Task |
| Befehl | `    RUN cmake -B build` | eingerückte Zeile innerhalb eines Tasks |


#### 2.1 Makros (`#define`)

```
#define SEG seg
```

Definiert das Wort `SEG` so, dass es überall dort, wo es als eigenständiges
Wort in einem Befehlsargument auftaucht, durch `seg` ersetzt wird — genau wie
ein C-Makro. Beispiel aus der Datei:

```
be:
    CD extensions/samfile-vscode
    RUN SEG build
```

Nach Makro-Expansion wird daraus effektiv `RUN seg build`. Das ist nützlich,
um z. B. den Namen eines Tools zentral an einer Stelle zu pflegen, falls es
sich mal ändert.


#### 2.2 Kommentare

Zeilen, die (nach optionalem Leerraum) mit `#` beginnen und nicht `#define`
sind, sind reine Kommentare:

```
# Run build Task
b:
    TASK build
```


#### 2.3 Tasks

Ein Task beginnt **in Spalte 0** (kein führendes Leerzeichen) mit einem Namen
gefolgt von einem Doppelpunkt:

```
build:
    RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    RUN ln -sf build/compile_commands.json compile_commands.json
    RUN cmake --build build
```

Task-Namen im Beispiel sind kurz und knapp — teils volle Wörter (`build`,
`debug`, `test`), teils bewusst kurze Aliase (`r`, `b`, `g`), vermutlich um
sie schnell auf der Kommandozeile eintippen zu können.


#### 2.4 Befehle

Jede eingerückte Zeile innerhalb eines Tasks beginnt mit einem
**Befehlswort in Großbuchstaben**, gefolgt von seinen Argumenten. Aus dem
Beispiel lassen sich diese Befehle ableiten:

| Befehl | Bedeutung | Beispiel |
|---|---|---|
| `RUN` | Shell-Befehl ausführen (plattformübergreifend) | `RUN cmake --build build` |
| `RUNWIN` | Shell-Befehl nur unter Windows ausführen | `RUNWIN ./out/build/x64-Debug/` |
| `TASK` | einen anderen Task aufrufen (wie `depends on`) | `TASK build` |
| `MKDIR` | Verzeichnis anlegen | `MKDIR release` |
| `MV` | Datei/Verzeichnis verschieben oder umbenennen | `MV NOTICE.md.html release/sfp-notice.html` |
| `CD` | Arbeitsverzeichnis für die folgenden Befehle wechseln | `CD extensions/samfile-vscode` |

`RUN` vs. `RUNWIN` legt nahe, dass das Tool plattformspezifische Befehle
unterstützt — vermutlich existiert analog auch etwas wie `RUNMAC` oder
`RUNLINUX`, das im Beispiel nur nicht vorkommt.


### 3. Vollständiges Beispiel mit Erklärung

```
#define SEG seg                      # Makro: SEG -> seg

r:                                   # Kurz-Alias
    TASK build                       #   ruft Task "build" auf

run:
    RUNWIN ./out/build/x64-Debug/    # nur unter Windows

# Build the it with CMake
build:
    RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    RUN ln -sf build/compile_commands.json compile_commands.json
    RUN cmake --build build

debug:
    RUN cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug
    RUN cmake --build build-debug -j

b:
    TASK build                       # weiterer Alias für "build"

g:
    RUN python build.py

gen:
    MKDIR release
    RUN ./build/sfp -pm NOTICE.iml NOTICE.md
    RUN npm i
    RUN npx sam-cli markdown NOTICE.md
    MV NOTICE.md.html release/sfp-notice.html
    MV NOTICE.md release/sfp-notice.md

test:
    RUN echo hello

be:
    CD extensions/samfile-vscode
    RUN SEG build                    # -> RUN seg build (nach Makro-Expansion)
```

Der Task-Graph (wer ruft wen über `TASK` auf) sieht daraus abgeleitet so aus:

```
r  --TASK--> build
b  --TASK--> build
```

Alle anderen Tasks (`run`, `debug`, `g`, `gen`, `test`, `be`) sind
eigenständig und rufen keine weiteren Tasks auf.


### 4. Typische Fehlerquellen

Diese Fehlerklassen ergeben sich direkt aus der Grammatik und werden vom
mitgelieferten `samfile_analyzer.py` geprüft:

1. **Doppelte Task-Namen** — zwei `build:`-Header überschreiben sich
   gegenseitig, meist unbeabsichtigt.
2. **Kaputte `TASK`-Referenzen** — `TASK buld` statt `TASK build` (Tippfehler)
   fällt sonst erst beim Ausführen auf.
3. **Zyklen** — `a: TASK b` und `b: TASK a` würde je nach Tool-Implementierung
   in eine Endlosschleife laufen.
4. **Leere Tasks** — ein `taskname:` ohne folgenden eingerückten Befehl tut
   nichts, ist aber leicht zu übersehen.
5. **Kleingeschriebene Befehle** — `run` statt `RUN` wird vermutlich vom
   echten Parser des Tools gar nicht erkannt.
6. **Uneinheitliche Einrückung** — Mischung aus Tabs und Leerzeichen
   innerhalb desselben Tasks kann je nach Parser-Implementierung zu
   Folgefehlern führen.
7. **Ungenutzte Makros** — ein `#define`, das nirgends referenziert wird, ist
   meist ein Hinweis auf toten Code oder einen Tippfehler beim Verwenden.


### 5. Einordnung: Wie verhält sich Samfile zu anderen Task-Runnern?

| Eigenschaft | Samfile (abgeleitet) | Make | Just | Task (Taskfile) |
|---|---|---|---|---|
| Syntax | eigenes, zeilenbasiertes Format | eigenes Format | eigenes Format | YAML |
| Abhängigkeiten zwischen Tasks | `TASK <name>` | `target: dependency` | `dep1 dep2:` | `deps: [..]` |
| Plattform-spezifische Befehle | `RUN` / `RUNWIN` | über Shell-Bedingungen | `[windows]`-Attribute | `platforms:` |
| Makros/Variablen | `#define NAME VALUE` | `NAME = value` | `name := value` | YAML-Variablen |
| Verzeichniswechsel | `CD <dir>` | meist `cd` in Shell-Zeile | `cd`-Rezept-Attribut | `dir:` |

Kurz gesagt: Samfile liest sich wie eine bewusst simplere, zeilenorientierte
Alternative zu Make — mit expliziten Befehlswörtern (`RUN`, `MKDIR`, `MV`,
`CD`) statt roher Shell-Syntax, was das Parsen (und damit auch den Bau eines
Analyzers/Editor-Plugins) deutlich einfacher macht als bei einem echten
Makefile.


### 6. Analyzer nutzen

```bash
python3 samfile_analyzer.py samfile
python3 samfile_analyzer.py samfile --json   # für Editor-/CI-Integration
```

Der Analyzer meldet Fehler, Warnungen und Hinweise mit Zeilennummer und
Fehlercode (z. B. `E008` für eine unbekannte `TASK`-Referenz) und gibt am
Ende eine Übersicht aller Tasks samt ihrer `TASK`-Aufrufe aus. Der Exit-Code
ist `1`, sobald mindestens ein Fehler (nicht nur Warnung/Hinweis) gefunden
wurde — praktisch für den Einsatz in einer CI-Pipeline oder als Pre-Commit-Hook.


## Version 2
