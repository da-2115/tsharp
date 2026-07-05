# T# VS Code Extension

Includes syntax highlighting, T# Dark theme, snippets, LSP client/server, built-in completions, stdlib completions, hover docs, signature help and semantic highlighting.

## Build

```bash
npm install
npm run compile
npx @vscode/vsce package
```

This creates a `.vsix` file.

## Install

```bash
code --install-extension tsharp-vscode-0.2.0.vsix
```
