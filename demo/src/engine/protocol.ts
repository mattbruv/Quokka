export type WorkerRequest =
  | { id: number; type: "init" }
  | { id: number; type: "go"; fen: string; depth: number };

export type WorkerResponse =
  | { id: number; type: "ready" }
  | { id: number; type: "result"; lines: string[]; elapsed: number }
  | { id: number; type: "error"; message: string };
