import * as vscode from "vscode";
import { execFile } from "child_process";


export function activate(context: vscode.ExtensionContext) {
    console.log("SAMFILE ANALYZER STARTED");


    const diagnosticCollection =
        vscode.languages.createDiagnosticCollection("samfile");


    function analyze(document: vscode.TextDocument) {

        execFile(
            "python3",
            [
                context.asAbsolutePath(
                    "analyzer/samfile_analyzer.py"
                ),
                "--json",
                document.fileName
            ],
            (error, stdout) => {

                if (!stdout)
                    return;


                const result =
                    JSON.parse(stdout);


                const diagnostics =
                    result.diagnostics.map((d: any) => {

                        return new vscode.Diagnostic(
                            new vscode.Range(
                                d.line - 1,
                                0,
                                d.line - 1,
                                100
                            ),
                            d.message,
                            severity(d.severity)
                        );

                    });


                diagnosticCollection.set(
                    document.uri,
                    diagnostics
                );
            }
        );
    }


    function severity(s: string) {

        switch (s) {
            case "error":
                return vscode.DiagnosticSeverity.Error;

            case "warning":
                return vscode.DiagnosticSeverity.Warning;

            default:
                return vscode.DiagnosticSeverity.Information;
        }
    }


    context.subscriptions.push(
        vscode.workspace.onDidOpenTextDocument(analyze)
    );


    context.subscriptions.push(
        vscode.workspace.onDidSaveTextDocument(analyze)
    );


    if (vscode.window.activeTextEditor) {
        analyze(
            vscode.window.activeTextEditor.document
        );
    }
}
