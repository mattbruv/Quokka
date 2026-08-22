import type { Color, Key } from "@lichess-org/chessground/types";
import { Chess, SQUARES } from "chess.js";
import type { PieceSymbol, Square } from "chess.js";

export const PROMOTION_CHOICES = [
  { role: "queen", symbol: "q" },
  { role: "knight", symbol: "n" },
  { role: "rook", symbol: "r" },
  { role: "bishop", symbol: "b" },
] as const;

export type PromotionSymbol = (typeof PROMOTION_CHOICES)[number]["symbol"];

export function toDests(chess: Chess): Map<Key, Key[]> {
  const dests = new Map<Key, Key[]>();
  for (const square of SQUARES) {
    const moves = chess.moves({ square, verbose: true });
    // Promotions appear once per piece; chessground only wants the square.
    if (moves.length) dests.set(square, [...new Set(moves.map((m) => m.to))]);
  }
  return dests;
}

export function toColor(chess: Chess): Color {
  return chess.turn() === "w" ? "white" : "black";
}

export function lastMoveKeys(chess: Chess): Key[] | undefined {
  const history = chess.history({ verbose: true });
  const last = history[history.length - 1];
  return last && [last.from, last.to];
}

/** The colour currently in check, for chessground's check highlight. */
export function checkedColor(chess: Chess): Color | undefined {
  return chess.isCheck() ? toColor(chess) : undefined;
}

export function isPromotionMove(chess: Chess, from: Square, to: Square): boolean {
  return chess
    .moves({ square: from, verbose: true })
    .some((m) => m.to === to && !!m.promotion);
}

export interface GameStatus {
  over: boolean;
  /** Short label for the status badge. */
  label: string;
  /** "1-0" | "0-1" | "1/2-1/2", only when the game is over. */
  result?: string;
}

export function gameStatus(chess: Chess): GameStatus {
  const mover = chess.turn() === "w" ? "White" : "Black";
  const opponent = chess.turn() === "w" ? "Black" : "White";

  if (chess.isCheckmate())
    return { over: true, label: `Checkmate — ${opponent} wins`, result: chess.turn() === "w" ? "0-1" : "1-0" };
  if (chess.isStalemate()) return { over: true, label: "Draw — stalemate", result: "1/2-1/2" };
  if (chess.isInsufficientMaterial())
    return { over: true, label: "Draw — insufficient material", result: "1/2-1/2" };
  if (chess.isThreefoldRepetition())
    return { over: true, label: "Draw — threefold repetition", result: "1/2-1/2" };
  if (chess.isDrawByFiftyMoves())
    return { over: true, label: "Draw — fifty-move rule", result: "1/2-1/2" };
  if (chess.isDraw()) return { over: true, label: "Draw", result: "1/2-1/2" };
  if (chess.isCheck()) return { over: false, label: `${mover} to move — check` };
  return { over: false, label: `${mover} to move` };
}

export interface MovePair {
  number: number;
  white?: string;
  black?: string;
}

/** Group the SAN history into numbered pairs for the move list. */
export function toMovePairs(chess: Chess): MovePair[] {
  const history = chess.history({ verbose: true });
  const pairs: MovePair[] = [];
  for (const move of history) {
    const last = pairs[pairs.length - 1];
    if (move.color === "w" || !last || last.black !== undefined) {
      pairs.push({
        number: pairs.length + 1,
        white: move.color === "w" ? move.san : undefined,
        black: move.color === "b" ? move.san : undefined,
      });
    } else {
      last.black = move.san;
    }
  }
  return pairs;
}

/** Split a UCI move like "e7e8q" into the pieces chess.js wants. */
export function parseUci(uci: string): { from: Square; to: Square; promotion?: PieceSymbol } | null {
  if (!/^[a-h][1-8][a-h][1-8][qrbn]?$/.test(uci)) return null;
  return {
    from: uci.slice(0, 2) as Square,
    to: uci.slice(2, 4) as Square,
    promotion: (uci[4] as PieceSymbol | undefined) ?? undefined,
  };
}

export function formatScore(score: number, isMate: boolean, pvLength: number): string {
  if (isMate) {
    const moves = Math.max(1, Math.ceil(pvLength / 2));
    return `${score > 0 ? "+" : "-"}M${moves}`;
  }
  const pawns = score / 100;
  return `${pawns > 0 ? "+" : ""}${pawns.toFixed(2)}`;
}

export function formatNodes(nodes: number): string {
  if (nodes >= 1_000_000) return `${(nodes / 1_000_000).toFixed(1)}M`;
  if (nodes >= 1_000) return `${(nodes / 1_000).toFixed(1)}k`;
  return String(nodes);
}
