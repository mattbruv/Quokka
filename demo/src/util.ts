import type { Api } from "@lichess-org/chessground/api";
import type { Color, Key } from "@lichess-org/chessground/types";
import { Chess, SQUARES } from "chess.js";

export function toDests(chess: Chess): Map<Key, Key[]> {
  const dests = new Map();
  SQUARES.forEach((s) => {
    const ms = chess.moves({ square: s, verbose: true });
    if (ms.length)
      dests.set(
        s,
        ms.map((m) => m.to)
      );
  });
  return dests;
}

export function toColor(chess: Chess): Color {
  return chess.turn() === "w" ? "white" : "black";
}

export function playOtherSide(cg: Api, chess: Chess) {
  return (orig: string, dest: string) => {
    chess.move({ from: orig, to: dest });
    cg.set({
      turnColor: toColor(chess),
      movable: {
        color: toColor(chess),
        dests: toDests(chess),
      },
    });
  };
}

export function aiPlay(
  cg: Api,
  chess: Chess,
  calculate: (gameFen: string) => Promise<string[]>,
  setState: (fen: string, pgn: string) => void,
  firstMove: boolean
) {
  return (orig: string, dest: string) => {
    setTimeout(async () => {
      chess.move({ from: orig, to: dest });
      setState(chess.fen(), chess.pgn());
      // get quokka output
      const output = await calculate(chess.fen());
      const best = output.find((x) => x.includes("bestmove"))?.split(" ")[1];
      console.log(best);

      const moves = chess.moves({ verbose: true });
      const move = moves.find((x) => x.lan === best)!;
      console.log(move);
      // const move = moves.find()
      // console.log(moves);
      // const move = firstMove
      //   ? moves[0]
      //   : moves[Math.floor(Math.random() * moves.length)];
      chess.move(move.san);
      cg.move(move.from, move.to);
      cg.set({
        turnColor: toColor(chess),
        movable: {
          color: toColor(chess),
          dests: toDests(chess),
        },
      });
      cg.playPremove();

      setState(chess.fen(), chess.pgn());
    }, 500);
  };
}
