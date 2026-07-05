import {
    createConnection, TextDocuments, ProposedFeatures, InitializeParams, TextDocumentSyncKind,
    CompletionItem, CompletionItemKind, Diagnostic, DiagnosticSeverity, Hover, MarkupKind,
    DocumentSymbol, SymbolKind, SemanticTokensBuilder, SemanticTokensLegend,
    SignatureHelp, SignatureInformation, ParameterInformation
} from "vscode-languageserver/node";
import { TextDocument } from "vscode-languageserver-textdocument";

const connection = createConnection(ProposedFeatures.all);
const documents: TextDocuments<TextDocument> = new TextDocuments(TextDocument);

const tokenTypes = ["namespace","type","class","interface","enum","function","method","property","variable","parameter","keyword","number","string","comment"];
const tokenModifiers: string[] = [];
const legend: SemanticTokensLegend = { tokenTypes, tokenModifiers };

const keywords = new Set(["import","class","interface","enum","public","private","protected","static","abstract","virtual","override","if","else","for","while","do","switch","case","default","break","continue","return","try","catch","throw","finally","this","base","true","false","null"]);
const primitiveTypes = new Set(["int","float","double","string","char","bool","void","any"]);

const builtins: Record<string, { kind: CompletionItemKind; detail: string; documentation: string; signature?: string }> = {
    print: { kind: CompletionItemKind.Function, detail: "print(value)", documentation: "Prints values without a newline.", signature: "print(any value)" },
    println: { kind: CompletionItemKind.Function, detail: "println(value)", documentation: "Prints values followed by a newline.", signature: "println(any value)" },
    size: { kind: CompletionItemKind.Function, detail: "size(value): int", documentation: "Returns the size of an array, string or runtime value.", signature: "size(any value): int" },
    typeof: { kind: CompletionItemKind.Function, detail: "typeof(value): string", documentation: "Returns the runtime type of a value.", signature: "typeof(any value): string" },
    address: { kind: CompletionItemKind.Function, detail: "address(value): string", documentation: "Returns the runtime memory address as hexadecimal.", signature: "address(any value): string" },
    push: { kind: CompletionItemKind.Function, detail: "push(array, value)", documentation: "Pushes a value onto an array.", signature: "push(array arr, any value)" },
    pop: { kind: CompletionItemKind.Function, detail: "pop(array): any", documentation: "Pops the last array element.", signature: "pop(array arr): any" },
    sort: { kind: CompletionItemKind.Function, detail: "sort(array)", documentation: "Sorts an array.", signature: "sort(array arr)" },
    sqrt: { kind: CompletionItemKind.Function, detail: "sqrt(x): double", documentation: "Square root.", signature: "sqrt(double x): double" },
    abs: { kind: CompletionItemKind.Function, detail: "abs(x): double", documentation: "Absolute value.", signature: "abs(double x): double" },
    pow: { kind: CompletionItemKind.Function, detail: "pow(x, y): double", documentation: "Power function.", signature: "pow(double x, double y): double" },
    sin: { kind: CompletionItemKind.Function, detail: "sin(x): double", documentation: "Sine in radians.", signature: "sin(double x): double" },
    cos: { kind: CompletionItemKind.Function, detail: "cos(x): double", documentation: "Cosine in radians.", signature: "cos(double x): double" },
    tan: { kind: CompletionItemKind.Function, detail: "tan(x): double", documentation: "Tangent in radians.", signature: "tan(double x): double" },
    atan2: { kind: CompletionItemKind.Function, detail: "atan2(y, x): double", documentation: "Angle from components.", signature: "atan2(double y, double x): double" },
    floor: { kind: CompletionItemKind.Function, detail: "floor(x): double", documentation: "Rounds down.", signature: "floor(double x): double" },
    ceil: { kind: CompletionItemKind.Function, detail: "ceil(x): double", documentation: "Rounds up.", signature: "ceil(double x): double" },
    round: { kind: CompletionItemKind.Function, detail: "round(x): double", documentation: "Rounds to nearest.", signature: "round(double x): double" },
    min: { kind: CompletionItemKind.Function, detail: "min(a, b): double", documentation: "Minimum.", signature: "min(double a, double b): double" },
    max: { kind: CompletionItemKind.Function, detail: "max(a, b): double", documentation: "Maximum.", signature: "max(double a, double b): double" },
    factorial: { kind: CompletionItemKind.Function, detail: "factorial(n): double", documentation: "Factorial.", signature: "factorial(int n): double" },
    pi: { kind: CompletionItemKind.Constant, detail: "pi: double", documentation: "Mathematical constant π." },
    e: { kind: CompletionItemKind.Constant, detail: "e: double", documentation: "Euler's number." },
    tau: { kind: CompletionItemKind.Constant, detail: "tau: double", documentation: "2π." },
    golden_ratio: { kind: CompletionItemKind.Constant, detail: "golden_ratio: double", documentation: "Golden ratio." }
};

const stdlibTypes: Record<string, { detail: string; documentation: string; methods: string[] }> = {
    Dictionary: { detail: "Dictionary<K,V>", documentation: "Generic key/value collection.", methods: ["put(key, value)", "get(key)", "contains_key(key)", "count()", "clear()"] },
    File: { detail: "File", documentation: "File helper class.", methods: ["read_all_text(path)", "write_all_text(path, text)", "append_all_text(path, text)", "exists(path)"] },
    Vector2: { detail: "Vector2<T>", documentation: "2D generic vector.", methods: ["add(other)", "subtract(other)", "multiply(scalar)", "divide(scalar)", "magnitude()", "length()", "normalized()", "dot(other)", "distance(other)", "angle()", "equals(other)", "to_string()"] },
    Vector3: { detail: "Vector3<T>", documentation: "3D generic vector.", methods: ["add(other)", "subtract(other)", "multiply(scalar)", "divide(scalar)", "magnitude()", "length()", "normalized()", "dot(other)", "cross(other)", "distance(other)", "equals(other)", "to_string()"] },
    Matrix2: { detail: "Matrix2<T>", documentation: "2x2 generic matrix.", methods: ["add(other)", "subtract(other)", "multiply(other)", "multiply_scalar(scalar)", "multiply_vector(v)", "determinant()", "transpose()", "to_string()"] },
    Matrix3: { detail: "Matrix3<T>", documentation: "3x3 generic matrix.", methods: ["add(other)", "subtract(other)", "multiply(other)", "multiply_scalar(scalar)", "multiply_vector(v)", "determinant()", "transpose()", "to_string()"] }
};

const importCompletions = ["import std.collections.Dictionary","import std.io.File","import std.math.Vector2","import std.math.Vector3","import std.math.Matrix2","import std.math.Matrix3"];

connection.onInitialize((_params: InitializeParams) => ({
    capabilities: {
        textDocumentSync: TextDocumentSyncKind.Incremental,
        completionProvider: { resolveProvider: false, triggerCharacters: [".", "<", " "] },
        hoverProvider: true,
        signatureHelpProvider: { triggerCharacters: ["(", ","] },
        documentSymbolProvider: true,
        semanticTokensProvider: { legend, range: false, full: true }
    }
}));

documents.onDidChangeContent(change => validateDocument(change.document));

function validateDocument(document: TextDocument): void {
    const diagnostics: Diagnostic[] = [];
    document.getText().split(/\r?\n/).forEach((line, i) => {
        const trimmed = line.trim();
        if (trimmed.startsWith("import ") && trimmed.endsWith(";")) {
            const c = line.indexOf(";");
            diagnostics.push({ severity: DiagnosticSeverity.Warning, range: { start: { line: i, character: c }, end: { line: i, character: c + 1 } }, message: "T# imports do not require semicolons.", source: "tsharp" });
        }
    });
    connection.sendDiagnostics({ uri: document.uri, diagnostics });
}

connection.onCompletion(params => {
    const document = documents.get(params.textDocument.uri);
    const text = document?.getText() ?? "";
    const line = document ? getLine(document, params.position.line) : "";
    const before = line.slice(0, params.position.character);

    if (before.trim().startsWith("import")) {
        return importCompletions.map(label => ({ label, kind: CompletionItemKind.Module, detail: "T# standard library import" }));
    }

    if (before.endsWith(".")) return memberCompletions(text, before);

    return [
        ...Object.entries(builtins).map(([label, meta]) => ({ label, kind: meta.kind, detail: meta.detail, documentation: { kind: MarkupKind.Markdown, value: meta.documentation } })),
        ...Object.entries(stdlibTypes).map(([label, meta]) => ({ label, kind: CompletionItemKind.Class, detail: meta.detail, documentation: { kind: MarkupKind.Markdown, value: meta.documentation } })),
        ...Array.from(primitiveTypes).map(label => ({ label, kind: CompletionItemKind.Keyword, detail: "T# primitive type" }))
    ];
});

function memberCompletions(documentText: string, before: string): CompletionItem[] {
    const match = before.match(/([A-Za-z_][A-Za-z0-9_]*)\.$/);
    if (!match) return [];
    const variableType = inferVariableType(documentText, match[1]);
    if (!variableType) return [];
    const baseType = variableType.replace(/<.*>/, "");
    const meta = stdlibTypes[baseType];
    if (!meta) return [];
    return meta.methods.map(method => ({ label: method.replace(/\(.*/, ""), kind: CompletionItemKind.Method, detail: method, documentation: { kind: MarkupKind.Markdown, value: `${baseType}.${method}` } }));
}

connection.onHover(params => {
    const document = documents.get(params.textDocument.uri);
    if (!document) return null;
    const word = getWordAt(document, params.position.line, params.position.character);
    if (!word) return null;

    if (builtins[word]) return { contents: { kind: MarkupKind.Markdown, value: `**${builtins[word].detail}**\n\n${builtins[word].documentation}` } };
    if (stdlibTypes[word]) return { contents: { kind: MarkupKind.Markdown, value: `**${stdlibTypes[word].detail}**\n\n${stdlibTypes[word].documentation}` } };
    if (primitiveTypes.has(word)) return { contents: { kind: MarkupKind.Markdown, value: `**${word}**\n\nPrimitive T# type.` } };
    return null;
});

connection.onSignatureHelp(params => {
    const document = documents.get(params.textDocument.uri);
    if (!document) return null;
    const before = getLine(document, params.position.line).slice(0, params.position.character);
    const match = before.match(/([A-Za-z_][A-Za-z0-9_]*)\s*\([^()]*$/);
    if (!match) return null;
    const builtin = builtins[match[1]];
    if (!builtin?.signature) return null;
    const sig = SignatureInformation.create(builtin.signature, builtin.documentation);
    const paramsText = builtin.signature.match(/\((.*)\)/)?.[1] ?? "";
    if (paramsText.trim()) sig.parameters = paramsText.split(",").map(p => ParameterInformation.create(p.trim()));
    return { signatures: [sig], activeSignature: 0, activeParameter: (before.match(/,/g) ?? []).length } as SignatureHelp;
});

connection.languages.semanticTokens.on(() => ({ data: [] }));

connection.onRequest("textDocument/semanticTokens/full", params => {
    const uri = (params as any).textDocument.uri as string;
    const document = documents.get(uri);
    if (!document) return { data: [] };

    const builder = new SemanticTokensBuilder();
    const lines = document.getText().split(/\r?\n/);

    for (let lineNo = 0; lineNo < lines.length; lineNo++) {
        const line = lines[lineNo];
        const commentIndex = line.indexOf("//");
        if (commentIndex >= 0) pushToken(builder, lineNo, commentIndex, line.length - commentIndex, "comment");
        tokenizeLine(builder, lineNo, commentIndex >= 0 ? line.slice(0, commentIndex) : line);
    }

    return builder.build();
});

function tokenizeLine(builder: SemanticTokensBuilder, lineNo: number, line: string) {
    let m: RegExpExecArray | null;

    const stringRegex = /"([^"\\]|\\.)*"|'([^'\\]|\\.)*'/g;
    while ((m = stringRegex.exec(line))) pushToken(builder, lineNo, m.index, m[0].length, "string");

    const numberRegex = /\b\d+(\.\d+)?\b/g;
    while ((m = numberRegex.exec(line))) pushToken(builder, lineNo, m.index, m[0].length, "number");

    const wordRegex = /\b[A-Za-z_][A-Za-z0-9_]*\b/g;
    while ((m = wordRegex.exec(line))) {
        const word = m[0], start = m.index;
        if (keywords.has(word)) pushToken(builder, lineNo, start, word.length, "keyword");
        else if (primitiveTypes.has(word)) pushToken(builder, lineNo, start, word.length, "type");
        else if (builtins[word]) pushToken(builder, lineNo, start, word.length, builtins[word].kind === CompletionItemKind.Constant ? "variable" : "function");
        else if (stdlibTypes[word]) pushToken(builder, lineNo, start, word.length, "class");
        else if (isDeclarationName(line, start, "class")) pushToken(builder, lineNo, start, word.length, "class");
        else if (isDeclarationName(line, start, "interface")) pushToken(builder, lineNo, start, word.length, "interface");
        else if (isDeclarationName(line, start, "enum")) pushToken(builder, lineNo, start, word.length, "enum");
        else if (line.slice(start + word.length).trimStart().startsWith("(")) {
            pushToken(builder, lineNo, start, word.length, line.slice(0, start).trimEnd().endsWith(".") ? "method" : "function");
        }
    }
}

connection.onDocumentSymbol(params => {
    const document = documents.get(params.textDocument.uri);
    if (!document) return [];
    const symbols: DocumentSymbol[] = [];
    document.getText().split(/\r?\n/).forEach((line, i) => {
        const classMatch = line.match(/^\s*class\s+([A-Za-z_][A-Za-z0-9_]*)/);
        const interfaceMatch = line.match(/^\s*interface\s+([A-Za-z_][A-Za-z0-9_]*)/);
        const enumMatch = line.match(/^\s*enum\s+([A-Za-z_][A-Za-z0-9_]*)/);
        const fnMatch = line.match(/^\s*(public|private|protected)?\s*(static\s+)?[A-Za-z_][A-Za-z0-9_<>,\[\]]*\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(/);
        if (classMatch) pushSymbol(symbols, line, i, classMatch[1], SymbolKind.Class);
        else if (interfaceMatch) pushSymbol(symbols, line, i, interfaceMatch[1], SymbolKind.Interface);
        else if (enumMatch) pushSymbol(symbols, line, i, enumMatch[1], SymbolKind.Enum);
        else if (fnMatch) pushSymbol(symbols, line, i, fnMatch[3], SymbolKind.Function);
    });
    return symbols;
});

function pushSymbol(symbols: DocumentSymbol[], line: string, i: number, name: string, kind: SymbolKind) {
    symbols.push({
        name, kind,
        range: { start: { line: i, character: 0 }, end: { line: i, character: line.length } },
        selectionRange: { start: { line: i, character: line.indexOf(name) }, end: { line: i, character: line.indexOf(name) + name.length } }
    });
}

function pushToken(builder: SemanticTokensBuilder, line: number, char: number, length: number, type: string) {
    const typeIndex = tokenTypes.indexOf(type);
    if (typeIndex >= 0 && length > 0) builder.push(line, char, length, typeIndex, 0);
}

function isDeclarationName(line: string, start: number, keyword: string): boolean {
    return line.slice(0, start).trim().endsWith(keyword);
}

function inferVariableType(text: string, variableName: string): string | null {
    const regex = new RegExp(`\\b([A-Za-z_][A-Za-z0-9_]*(?:<[^>]+>)?)\\s+${variableName}\\b`);
    const match = text.match(regex);
    return match ? match[1] : null;
}

function getLine(document: TextDocument, line: number): string {
    return document.getText({ start: { line, character: 0 }, end: { line, character: Number.MAX_SAFE_INTEGER } });
}

function getWordAt(document: TextDocument, line: number, character: number): string | null {
    const text = getLine(document, line);
    const left = text.slice(0, character).match(/[A-Za-z_][A-Za-z0-9_]*$/)?.[0] ?? "";
    const right = text.slice(character).match(/^[A-Za-z0-9_]*/)?.[0] ?? "";
    const word = left + right;
    return word.length ? word : null;
}

documents.listen(connection);
connection.listen();
