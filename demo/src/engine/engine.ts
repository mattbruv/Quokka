import type { WorkerRequest, WorkerResponse } from "./protocol.ts";

/** `Omit` collapses unions, so distribute over the request variants. */
type DistributiveOmit<T, K extends PropertyKey> = T extends unknown ? Omit<T, K> : never;

export interface EngineInfo {
  /** Centipawns from the moving side's point of view, or a mate score. */
  score: number;
  depth: number;
  nodes: number;
  /** Milliseconds spent in the search, measured in the worker. */
  time: number;
  pv: string[];
}

export interface SearchResult {
  bestMove: string | null;
  info: EngineInfo | null;
  lines: string[];
}

/** Scores at or beyond this magnitude are forced mates, not evaluations. */
const MATE_THRESHOLD = 90000;

export function isMateScore(score: number): boolean {
  return Math.abs(score) >= MATE_THRESHOLD;
}

function parseInfoLine(line: string): EngineInfo | null {
  const tokens = line.trim().split(/\s+/);
  if (tokens[0] !== "info") return null;

  const info: EngineInfo = { score: 0, depth: 0, nodes: 0, time: 0, pv: [] };
  for (let i = 1; i < tokens.length; i++) {
    switch (tokens[i]) {
      case "cp":
        info.score = Number(tokens[++i]);
        break;
      case "depth":
        info.depth = Number(tokens[++i]);
        break;
      case "nodes":
        info.nodes = Number(tokens[++i]);
        break;
      case "time":
        info.time = Number(tokens[++i]);
        break;
      case "pv":
        info.pv = tokens.slice(i + 1);
        i = tokens.length;
        break;
    }
  }
  return info;
}

function parseOutput(lines: string[], elapsed: number): SearchResult {
  let info: EngineInfo | null = null;
  let bestMove: string | null = null;

  for (const line of lines) {
    if (line.startsWith("bestmove")) {
      const move = line.trim().split(/\s+/)[1];
      // The engine emits a null move ("a1a1") when it has nothing to play.
      bestMove = move && move !== "a1a1" ? move : null;
    } else {
      // Later `info` lines come from deeper iterations, so the last one wins.
      info = parseInfoLine(line) ?? info;
    }
  }
  // The engine never initialises its own search clock, so trust the worker's.
  if (info) info.time = elapsed;
  return { bestMove, info, lines };
}

/**
 * Promise-based client for the search worker. One search runs at a time;
 * `restart` throws away a worker mid-think so the UI never has to wait for a
 * search it no longer cares about.
 */
export class Engine {
  private worker: Worker | null = null;
  private seq = 0;
  private pending = new Map<
    number,
    { resolve: (r: SearchResult) => void; reject: (e: Error) => void }
  >();

  private spawn(): Worker {
    const worker = new Worker(new URL("./worker.ts", import.meta.url), { type: "module" });
    worker.addEventListener("message", (e: MessageEvent<WorkerResponse>) => {
      const msg = e.data;
      const entry = this.pending.get(msg.id);
      if (!entry) return;
      this.pending.delete(msg.id);
      if (msg.type === "error") entry.reject(new Error(msg.message));
      else if (msg.type === "result") entry.resolve(parseOutput(msg.lines, msg.elapsed));
      else entry.resolve({ bestMove: null, info: null, lines: [] });
    });
    worker.addEventListener("error", (e) => {
      this.rejectAll(new Error(e.message || "engine worker crashed"));
    });
    return worker;
  }

  private rejectAll(error: Error) {
    for (const { reject } of this.pending.values()) reject(error);
    this.pending.clear();
  }

  private send(request: DistributiveOmit<WorkerRequest, "id">): Promise<SearchResult> {
    this.worker ??= this.spawn();
    const id = ++this.seq;
    const worker = this.worker;
    return new Promise<SearchResult>((resolve, reject) => {
      this.pending.set(id, { resolve, reject });
      worker.postMessage({ ...request, id } as WorkerRequest);
    });
  }

  /** Warm up the WASM module so the first move is not charged for the load. */
  preload(): Promise<SearchResult> {
    return this.send({ type: "init" });
  }

  search(fen: string, depth: number): Promise<SearchResult> {
    return this.send({ type: "go", fen, depth });
  }

  /** Abandon an in-flight search. The next request spawns a fresh worker. */
  restart() {
    this.worker?.terminate();
    this.worker = null;
    this.rejectAll(new Error("aborted"));
  }

  destroy() {
    this.restart();
  }
}
