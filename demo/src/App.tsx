import { useEffect, useRef, useState } from "react";
//@ts-expect-error no types
import createQuokka from "./quokka.js";
import { Button, Input, NumberInput, Stack } from "@chakra-ui/react";

import { Chessground } from "@lichess-org/chessground";

import "@lichess-org/chessground/assets/chessground.base.css";
import "./css/chessground.brown.css";
import "./css/chessground.cburnett.css";

import type { Api } from "@lichess-org/chessground/api";
import { Chess } from "chess.js";
import { aiPlay, playOtherSide, toDests } from "./util.js";

function App() {
  const [output, setOutput] = useState<string[]>([]);
  const [fen, setFen] = useState<string>(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
  );
  const [depth, setDepth] = useState<number>(5);
  const [ground, setGround] = useState<Api | null>(null);

  const chess = new Chess();

  const groundRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (groundRef.current) {
      setGround(
        Chessground(groundRef.current, {
          movable: {
            free: false, // only allow legal moves
            color: "both", // white pieces can move
            dests: toDests(chess),
          },
          draggable: {
            showGhost: true,
          },
          events: {
            move(orig, dest, capturedPiece) {
              chess.move({ from: orig, to: dest });
              console.log(chess);
              ground?.set({
                movable: {
                  free: true,
                  color: "both",
                  dests: toDests(chess),
                },
              });
              console.log(orig, dest, capturedPiece);
            },
          },
          fen,
        })
      );
    }
  }, []);

  async function calculate() {
    const module = await createQuokka();
    // takes a depth and FEN, returns program output
    const int_sqrt = module.cwrap("int_sqrt", "string", ["number", "string"]);
    const stdout: string = int_sqrt(depth, fen);
    setOutput(stdout.split("\n"));
  }

  return (
    <>
      <Stack>
        <div style={{ width: 400, height: 400 }} ref={groundRef}></div>
        <div>
          <Input
            onChange={(x) => setFen(x.target.value)}
            value={fen}
            placeholder="Enter position FEN"
          />
          <NumberInput.Root
            value={depth.toString()}
            onValueChange={(e) => setDepth(Number(e.value))}
            defaultValue="3"
            width="200px"
          >
            <NumberInput.Control />
            <NumberInput.Input />
          </NumberInput.Root>
          <Button onClick={calculate}>Calculate</Button>
          {output
            // .filter((x) => x.includes("bestmove"))
            .map((x) => (
              <div key={x}>{x}</div>
            ))}
        </div>
      </Stack>
    </>
  );
}

export default App;
