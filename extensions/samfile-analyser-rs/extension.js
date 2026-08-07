const vscode = require("vscode");
const path = require("path");
const lc = require("vscode-languageclient/node");

function activate(context) {
  const serverExe = context.asAbsolutePath(
    path.join("target", "debug", "samfile-lsp"),
  );

  const serverOptions = {
    command: serverExe,
  };

  const clientOptions = {
    documentSelector: [{ scheme: "file", language: "samfile" }],
    traceOutputChannel: vscode.window.createOutputChannel("Samfile LSP Trace", {
      log: true,
    }),
  };

  const client = new lc.LanguageClient(
    "samfile-lsp",
    "Samfile LSP",
    serverOptions,
    clientOptions,
  );

  context.subscriptions.push(client.start());
}

exports.activate = activate;
