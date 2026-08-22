/**
 * Runs the Quokka WASM search off the main thread.
 *
 * The search is a blocking C++ call, so running it inline would freeze the
 * board for the whole think. The module is instantiated once and reused.
 */
//@ts-expect-error no types
import createQuokka from "../quokka.js";

import type { WorkerRequest, WorkerResponse } from "./protocol.ts";

type SearchFn = (depth: number, fen: string) => string;

const ctx = self as unknown as {
  addEventListener(t: "message", cb: (e: MessageEvent<WorkerRequest>) => void): void;
  postMessage(m: WorkerResponse): void;
};

let searchPromise: Promise<SearchFn> | null = null;

function getSearch(): Promise<SearchFn> {
  return (searchPromise ||= createQuokka().then(
    (module: { cwrap: (...a: unknown[]) => SearchFn }) =>
      module.cwrap("int_sqrt", "string", ["number", "string"])
  ));
}

ctx.addEventListener("message", (e) => {
  const req = e.data;
  void (async () => {
    try {
      const search = await getSearch();
      if (req.type === "init") {
        ctx.postMessage({ id: req.id, type: "ready" });
        return;
      }
      const started = performance.now();
      const raw = search(req.depth, req.fen);
      const elapsed = Math.round(performance.now() - started);
      const lines = raw.split("\n").filter((l) => l.trim() !== "");
      ctx.postMessage({ id: req.id, type: "result", lines, elapsed });
    } catch (err) {
      ctx.postMessage({ id: req.id, type: "error", message: String(err) });
    }
  })();
});
