#!/usr/bin/env python3
"""
samfile_analyzer.py — statischer Analyzer für "Samfile"-Task-Runner-Configs.

Architektur (angelehnt an rust-analyzer, aber schlank):

    Text  --Lexer-->  Zeilen-Tokens  --Parser-->  AST (Defines, Tasks, Commands)
                                                        |
                                                        v
                                              Diagnostics-Passes
                                                        |
                                                        v
                                        Report (Text oder JSON für Editoren)

Es gibt keine öffentliche Spezifikation für dieses Format — die Grammatik
wurde aus einer Beispieldatei abgeleitet. Bekannte Elemente:

  #define NAME VALUE     -> Makro-Definition (wortweise Ersetzung, wie C-Macros)
  # Kommentar             -> Kommentarzeile
  taskname:               -> Task-Definition (Spalte 0, kein Leerzeichen davor)
      COMMAND arg...       -> eingerückter Befehl innerhalb des Tasks

  Bekannte Befehle: TASK, RUN, RUNWIN, MKDIR, MV, CD
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass, field
from enum import Enum


KNOWN_COMMANDS = {
    "CD",
    "CDWIN",
    "CDMAC",
    "CDLIN",
    "ENV",
    "ENVWIN",
    "ENVMAC",
    "ENVLIN",
    "RUN",
    "RUNWIN",
    "RUNMAC",
    "RUNLIN",
    "TASK",
    "TASKWIN",
    "TASKMAC",
    "TASKLIN",
    "RM",
    "RMWIN",
    "RMMAC",
    "RMLIN",
    "MKDIR",
    "MKDIRWIN",
    "MKDIRMAC",
    "MKDIRLIN",
    "CP",
    "CPWIN",
    "CPMAC",
    "CPLIN",
    "MV",
    "MVWIN",
    "MVMAC",
    "MVLIN",
    "SLEEP",
    "SLEEPWIN",
    "SLEEPMAC",
    "SLEEPLIN",
    "SHELL",
    "SHELLWIN",
    "SHELLMAC",
    "SHELLLIN",
    "ECHO",
    "ECHOWIN",
    "ECHOMAC",
    "ECHOLIN",
    "WARN",
    "WARNWIN",
    "WARNMAC",
    "WARNLIN",
    "ERROR",
    "ERRORWIN",
    "ERRORMAC",
    "ERRORLIN",
    "TOUCH",
    "TOUCHWIN",
    "TOUCHMAC",
    "TOUCHLIN",
    "WRITE",
    "WRITEWIN",
    "WRITEMAC",
    "WRITELIN",
    "APPEND",
    "APPENDWIN",
    "APPENDMAC",
    "APPENDLIN",
    "UNSETENV",
    "UNSETENVWIN",
    "UNSETENVMAC",
    "UNSETENVLIN",
    "PROMPT",
    "PROMPTWIN",
    "PROMPTMAC",
    "PROMPTLIN",
}
TASK_NAME_RE = re.compile(r"^[A-Za-z0-9_-]+$")
MACRO_NAME_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")


class Severity(str, Enum):
    ERROR = "error"
    WARNING = "warning"
    INFO = "info"


@dataclass
class Diagnostic:
    severity: Severity
    code: str
    line: int
    message: str

    def to_dict(self):
        return {
            "severity": self.severity.value,
            "code": self.code,
            "line": self.line,
            "message": self.message,
        }


@dataclass
class Command:
    line: int
    keyword: str
    args: str
    indent: str  # raw whitespace prefix, for indentation checks


@dataclass
class Task:
    name: str
    header_line: int
    commands: list[Command] = field(default_factory=list)


@dataclass
class Macro:
    name: str
    value: str
    line: int
    used: bool = False


@dataclass
class ParseResult:
    macros: dict[str, Macro]
    tasks: dict[str, Task]
    task_order: list[str]
    diagnostics: list[Diagnostic]


def parse(text: str) -> ParseResult:
    diagnostics: list[Diagnostic] = []
    macros: dict[str, Macro] = {}
    tasks: dict[str, Task] = {}
    task_order: list[str] = []

    current_task: Task | None = None

    lines = text.splitlines()
    for lineno, raw in enumerate(lines, start=1):
        if raw.strip() == "":
            continue

        # Directive / comment (must start at column 0, no leading whitespace)
        if not raw[0].isspace():
            stripped = raw.strip()

            if stripped.startswith("#define"):
                parts = stripped.split(None, 2)
                if len(parts) < 3:
                    diagnostics.append(Diagnostic(
                        Severity.ERROR, "E001", lineno,
                        f"'#define' braucht Name und Wert, gefunden: {stripped!r}"
                    ))
                else:
                    _, name, value = parts
                    if not MACRO_NAME_RE.match(name):
                        diagnostics.append(Diagnostic(
                            Severity.ERROR, "E002", lineno,
                            f"Ungültiger Makroname '{name}' "
                            f"(erlaubt: Buchstaben, Ziffern, '_', nicht mit Ziffer beginnend)"
                        ))
                    elif name in macros:
                        diagnostics.append(Diagnostic(
                            Severity.WARNING, "W001", lineno,
                            f"Makro '{name}' wurde bereits in Zeile "
                            f"{macros[name].line} definiert und wird überschrieben"
                        ))
                    macros[name] = Macro(name=name, value=value, line=lineno)
                current_task = None
                continue

            if stripped.startswith("#"):
                # normaler Kommentar
                current_task = None
                continue

            # Task-Header: "name:" oder "name: trailing"
            if ":" in stripped:
                name, _, trailing = stripped.partition(":")
                name = name.strip()
                trailing = trailing.strip()

                if not TASK_NAME_RE.match(name):
                    diagnostics.append(Diagnostic(
                        Severity.ERROR, "E003", lineno,
                        f"Ungültiger Task-Name '{name}' "
                        f"(erlaubt: Buchstaben, Ziffern, '_', '-')"
                    ))

                if trailing:
                    diagnostics.append(Diagnostic(
                        Severity.WARNING, "W002", lineno,
                        f"Unerwarteter Inhalt nach ':' in Task-Header: {trailing!r} "
                        f"wird ignoriert"
                    ))

                if name in tasks:
                    diagnostics.append(Diagnostic(
                        Severity.ERROR, "E004", lineno,
                        f"Task '{name}' ist bereits in Zeile "
                        f"{tasks[name].header_line} definiert (Duplikat)"
                    ))
                    # trotzdem weiterparsen, überschreibt aber nicht das Original
                    current_task = tasks[name]
                else:
                    current_task = Task(name=name, header_line=lineno)
                    tasks[name] = current_task
                    task_order.append(name)
                continue

            diagnostics.append(Diagnostic(
                Severity.ERROR, "E005", lineno,
                f"Zeile weder Kommentar, '#define' noch Task-Header: {stripped!r}"
            ))
            current_task = None
            continue

        # Eingerückte Zeile -> Command (oder Kommentar) innerhalb eines Tasks
        indent = raw[: len(raw) - len(raw.lstrip())]
        stripped = raw.strip()

        if stripped.startswith("#"):
            continue  # eingerückter Kommentar, ok

        if current_task is None:
            diagnostics.append(Diagnostic(
                Severity.ERROR, "E006", lineno,
                f"Eingerückte Zeile ohne vorausgehenden Task-Header: {stripped!r}"
            ))
            continue

        parts = stripped.split(None, 1)
        keyword = parts[0]
        args = parts[1] if len(parts) > 1 else ""

        if keyword != keyword.upper():
            diagnostics.append(Diagnostic(
                Severity.WARNING, "W003", lineno,
                f"Befehl '{keyword}' sollte in Großbuchstaben stehen "
                f"(z.B. '{keyword.upper()}')"
            ))
        elif keyword not in KNOWN_COMMANDS:
            diagnostics.append(Diagnostic(
                Severity.INFO, "I001", lineno,
                f"Unbekannter Befehl '{keyword}' (bekannt: "
                f"{', '.join(sorted(KNOWN_COMMANDS))}) — evtl. Tippfehler oder "
                f"projektspezifische Erweiterung"
            ))

        if keyword in ("RUN", "RUNWIN", "CD", "MKDIR", "MV") and not args.strip():
            diagnostics.append(Diagnostic(
                Severity.ERROR, "E007", lineno,
                f"Befehl '{keyword}' erwartet ein Argument, aber keins wurde angegeben"
            ))

        current_task.commands.append(
            Command(line=lineno, keyword=keyword, args=args, indent=indent)
        )

    # ---- Passes, die den vollständigen Baum brauchen ----
    _check_empty_tasks(tasks, diagnostics)
    _check_indentation_consistency(tasks, diagnostics)
    _check_task_references(tasks, diagnostics)
    _check_task_cycles(tasks, diagnostics)
    _check_macro_usage(tasks, macros, diagnostics)

    diagnostics.sort(key=lambda d: d.line)
    return ParseResult(macros=macros, tasks=tasks, task_order=task_order,
                        diagnostics=diagnostics)


def _check_empty_tasks(tasks: dict[str, Task], diagnostics: list[Diagnostic]) -> None:
    for task in tasks.values():
        if not task.commands:
            diagnostics.append(Diagnostic(
                Severity.WARNING, "W004", task.header_line,
                f"Task '{task.name}' hat keinen Befehl (leerer Task-Body)"
            ))


def _check_indentation_consistency(tasks: dict[str, Task],
                                    diagnostics: list[Diagnostic]) -> None:
    for task in tasks.values():
        indents = {c.indent for c in task.commands}
        if len(indents) <= 1:
            continue
        # gemischte Einrückungen (z.B. Tabs und Spaces, oder unterschiedliche Breiten)
        first_cmd = task.commands[0]
        diagnostics.append(Diagnostic(
            Severity.WARNING, "W005", first_cmd.line,
            f"Task '{task.name}' verwendet uneinheitliche Einrückung "
            f"innerhalb seiner Befehle ({len(indents)} verschiedene Varianten)"
        ))


def _check_task_references(tasks: dict[str, Task],
                            diagnostics: list[Diagnostic]) -> None:
    for task in tasks.values():
        for cmd in task.commands:
            if cmd.keyword != "TASK":
                continue
            target = cmd.args.strip()
            if not target:
                continue  # bereits als E007 gemeldet
            if target not in tasks:
                diagnostics.append(Diagnostic(
                    Severity.ERROR, "E008", cmd.line,
                    f"TASK verweist auf unbekannten Task '{target}'"
                ))
            elif target == task.name:
                diagnostics.append(Diagnostic(
                    Severity.ERROR, "E009", cmd.line,
                    f"Task '{task.name}' ruft sich direkt selbst auf (TASK {target})"
                ))


def _check_task_cycles(tasks: dict[str, Task], diagnostics: list[Diagnostic]) -> None:
    graph = {
        name: [c.args.strip() for c in t.commands
               if c.keyword == "TASK" and c.args.strip() in tasks]
        for name, t in tasks.items()
    }

    WHITE, GRAY, BLACK = 0, 1, 2
    color = {name: WHITE for name in tasks}
    reported: set[frozenset] = set()

    def dfs(node: str, stack: list[str]) -> None:
        color[node] = GRAY
        stack.append(node)
        for neighbor in graph.get(node, []):
            if color[neighbor] == GRAY:
                cycle = stack[stack.index(neighbor):] + [neighbor]
                key = frozenset(cycle)
                if key not in reported:
                    reported.add(key)
                    diagnostics.append(Diagnostic(
                        Severity.ERROR, "E010", tasks[node].header_line,
                        f"Zyklische TASK-Aufrufe entdeckt: {' -> '.join(cycle)}"
                    ))
            elif color[neighbor] == WHITE:
                dfs(neighbor, stack)
        stack.pop()
        color[node] = BLACK

    for name in tasks:
        if color[name] == WHITE:
            dfs(name, [])


WORD_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")


def _check_macro_usage(tasks: dict[str, Task], macros: dict[str, Macro],
                        diagnostics: list[Diagnostic]) -> None:
    for task in tasks.values():
        for cmd in task.commands:
            for word in WORD_RE.findall(cmd.args):
                if word in macros:
                    macros[word].used = True

    for macro in macros.values():
        if not macro.used:
            diagnostics.append(Diagnostic(
                Severity.INFO, "I002", macro.line,
                f"Makro '{macro.name}' wird nirgends verwendet"
            ))


def expand_macros(text: str, macros: dict[str, Macro]) -> str:
    def repl(m: re.Match) -> str:
        word = m.group(0)
        return macros[word].value if word in macros else word

    return WORD_RE.sub(repl, text)


# --------------------------------------------------------------------------
# Reporting
# --------------------------------------------------------------------------

SEVERITY_LABEL = {
    Severity.ERROR: "error  ",
    Severity.WARNING: "warning",
    Severity.INFO: "info   ",
}


def print_report(path: str, result: ParseResult) -> int:
    n_err = sum(1 for d in result.diagnostics if d.severity == Severity.ERROR)
    n_warn = sum(1 for d in result.diagnostics if d.severity == Severity.WARNING)
    n_info = sum(1 for d in result.diagnostics if d.severity == Severity.INFO)

    print(f"samfile-analyzer: {path}")
    print(f"  {len(result.tasks)} Task(s), {len(result.macros)} Makro(s)\n")

    if not result.diagnostics:
        print("  Keine Auffälligkeiten gefunden. ✔\n")
    for d in result.diagnostics:
        print(f"  {path}:{d.line}: {SEVERITY_LABEL[d.severity]} [{d.code}] {d.message}")

    print(f"\n  Summe: {n_err} Fehler, {n_warn} Warnungen, {n_info} Hinweise")

    print("\n  Task-Übersicht:")
    for name in result.task_order:
        task = result.tasks[name]
        calls = [c.args.strip() for c in task.commands if c.keyword == "TASK"]
        suffix = f"  (ruft auf: {', '.join(calls)})" if calls else ""
        print(f"    - {name}  [{len(task.commands)} Befehl(e)]{suffix}")

    return 1 if n_err else 0


def main() -> int:
    ap = argparse.ArgumentParser(description="Analyzer für Samfile-Task-Configs")
    ap.add_argument("path", help="Pfad zum Samfile")
    ap.add_argument("--json", action="store_true",
                     help="Diagnostics als JSON ausgeben (z.B. für Editor-Integration)")
    args = ap.parse_args()

    with open(args.path, "r", encoding="utf-8") as f:
        text = f.read()

    result = parse(text)

    if args.json:
        payload = {
            "path": args.path,
            "tasks": {
                name: {
                    "line": t.header_line,
                    "commands": [
                        {"line": c.line, "keyword": c.keyword, "args": c.args}
                        for c in t.commands
                    ],
                }
                for name, t in result.tasks.items()
            },
            "macros": {
                name: {"value": m.value, "line": m.line, "used": m.used}
                for name, m in result.macros.items()
            },
            "diagnostics": [d.to_dict() for d in result.diagnostics],
        }
        print(json.dumps(payload, indent=2, ensure_ascii=False))
        return 1 if any(d.severity == Severity.ERROR for d in result.diagnostics) else 0

    return print_report(args.path, result)


if __name__ == "__main__":
    sys.exit(main())
