# fling-lsp

Ein Language Server (LSP) für **Fling** in Rust, gebaut mit [`tower-lsp`](https://github.com/ebkalderon/tower-lsp) und `tokio`, auf Basis von `fling-language-spec.md`.

Da der Referenz-Interpreter Fehler meist "leise" toleriert (still `Null` zurückgibt, kein Crash), macht dieser LSP eine **eigenständige statische Analyse**, die strenger ist als die Laufzeit — inklusive der in der Spezifikation dokumentierten Bugs (B1–B17), damit sie in Diagnostics/Hover sichtbar werden statt den Nutzer im Dunkeln zu lassen.

## Aufbau

```
src/
  lexer.rs       Tokenizer (Spec §2) inkl. Quirks (kein '_', keine Escapes, kein Float, ...)
  ast.rs         AST-Knoten (Spec §4), mit sauberen Feldnamen statt der C++-Tippfehler
  parser.rs      Rekursiver Abstiegsparser (Spec §3) mit Fehlerrecovery statt exit(1)
  diagnostics.rs Gemeinsamer Diag-Typ (Severity, Code, Span)
  analysis.rs    Scope-/Symboltabelle (Spec §6.2, inkl. Bug B8: if/else kein eigener Scope),
                 alle semantischen Diagnostics, Typinferenz-Heuristik, Hover-Text für Operatoren
  server.rs      tower-lsp `LanguageServer`-Implementierung (Diagnostics, Hover, Go-to-Definition,
                 Find References, Completion, Document Symbols)
  main.rs        Startet den Server über stdio
  lib.rs         Re-exportiert alle Module (für Tests/Tools)
examples/
  check.rs       Kleines CLI-Tool: `cargo run --example check -- datei.fling` druckt alle
                 Diagnostics ohne LSP-Client (praktisch zum Debuggen/Verifizieren)
```

## Bauen & Ausführen

```bash
cargo build --release
# Binary liegt danach unter target/release/fling-lsp
```

Der Server kommuniziert über **stdio** (Standard-LSP-Transport) — er wird nicht direkt aufgerufen, sondern von einem Editor/Client gestartet.

### Schnelltest ohne Editor

```bash
cargo run --example check -- pfad/zur/datei.fling
```
gibt alle Diagnostics (Zeile:Spalte, Severity, Code, Nachricht) auf der Konsole aus.

## Implementierte Features

- **Diagnostics** (Spec §11.1): Syntaxfehler mit Zeile/Spalte, undeklarierte Variablen/Funktionen,
  Redeklaration im selben Scope, Zuweisung an `const`, Zuweisung an Member/Index-Ausdrücke (B6),
  fehlendes `;` nach `let`/`const`, überflüssiges `;` nach Ausdrucks-Statements (B14),
  Aufrufverkettung `foo().bar` (B13), `else if` (ungültig laut Grammatik), String-Konkatenation
  mit `+` (B3), Computed Object-Access (B4), `string[index]` (B5), Dezimalpunkt-Literale (B2),
  falsche Argumentanzahl bei bekannten Funktionen (optionale Warnung).
- **Hover**: Operator-Semantik für `%` (B1, vertauschte Operanden), `/` (B12, Division durch 0 → 0),
  `+` (B3), Vergleichsoperatoren; für Identifier: Art (`let`/`const`/Parameter/Funktion/Global),
  inferierter Typ (best effort), bekannte Objekt-Properties, implizite Return-Regel bei Funktionen.
- **Go to Definition** / **Find References**: über die nachgebaute Scope-Kette (While-Bodies und
  Funktionsaufrufe bekommen eigene Scopes, `if`/`else` **nicht** — Bug B8 wird korrekt reproduziert).
- **Completion**: Keywords (`let const fn if else while`), globale Vordeklarationen
  (`true false null print`), lokale Symbole entlang der Scope-Kette, `.length` und bekannte
  Objekt-Property-Namen nach `.`.
- **Document Symbols**: Übersicht aller `let`/`const`/`fn`-Deklarationen einer Datei.

## Nicht (bzw. noch nicht) implementiert

- Formatter / Code Actions (Spec §11.6, §14 Phase 5) — als nächster Ausbauschritt vorgesehen
  (z. B. automatisches Umschreiben von `else if` → `else { if ... }`, Entfernen von B14-Semikola).
- Semantic Tokens / eigenes Syntax-Highlighting (§11.5) — viele Editoren bringen für C-ähnliche
  Syntax bereits brauchbares TextMate-Highlighting mit, daher niedrigere Priorität.

## Editor-Anbindung (Beispiel: VS Code / beliebiger LSP-Client)

Praktisch jeder Editor mit generischer LSP-Unterstützung (z. B. Neovim, Helix, VS Code via einer
kleinen Extension oder `vscode-languageclient`) kann `target/release/fling-lsp` als Sprachserver
für `*.fling`-Dateien registrieren; der Server erwartet Standard-stdio-JSON-RPC, keine
Kommandozeilenargumente.
