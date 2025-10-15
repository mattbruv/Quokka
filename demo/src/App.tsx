import { useEffect, useRef, useState } from "react";
//@ts-expect-error no types
import createQuokka from "./quokka.js";
import { Box, Button, Flex, Input, Stack } from "@chakra-ui/react";

import { Chessground } from "@lichess-org/chessground";

import "@lichess-org/chessground/assets/chessground.base.css";
import "./css/chessground.brown.css";
import "./css/chessground.cburnett.css";

// import type { Api } from "@lichess-org/chessground/api";
import { Chess } from "chess.js";
import { aiPlay, toDests } from "./util.js";

let chess = new Chess();

function App() {
  const [output, setOutput] = useState<string[]>([]);
  const [fen, setFen] = useState<string>(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
  );
  const [depth, _setDepth] = useState<number>(5);
  //const [ground, setGround] = useState<Api | null>(null);
  const [pgn, setPgn] = useState("");
  // const [side, setSide] = useState<"white" | "black">("white");

  const groundRef = useRef<HTMLDivElement>(null);

  // function updateSide() {
  //   setSide(side === "black" ? "white" : "black");
  //   console.log(ground);
  //   ground?.set({
  //     movable: {
  //       color: "black",
  //       events: {
  //         after: aiPlay(ground, chess, calculate, (fen, pgn) => {
  //           setFen(fen);
  //           setPgn(pgn);
  //         }),
  //       },
  //     },
  //   });
  // }

  useEffect(() => {
    if (groundRef.current) {
      const ground = Chessground(groundRef.current, {
        premovable: {
          enabled: false,
        },
        movable: {
          free: false, // only allow legal moves
          color: "white", // white pieces can move
          dests: toDests(chess),
        },
        draggable: {
          showGhost: true,
        },
        fen,
      });

      ground.set({
        movable: {
          events: {
            after: aiPlay(ground, chess, calculate, (fen, pgn) => {
              setFen(fen);
              setPgn(pgn);
            }),
          },
        },
      });
    }
  }, []);

  // function reset() {
  //   chess = new Chess();
  //   ground?.set({ fen: chess.fen() });
  // }

  async function calculate(gameFen: string): Promise<string[]> {
    const module = await createQuokka();
    // takes a depth and FEN, returns program output
    const int_sqrt = module.cwrap("int_sqrt", "string", ["number", "string"]);
    const stdout: string = int_sqrt(depth, gameFen);
    const lines = stdout.split("\n");
    setOutput(lines);
    return lines;
  }

  async function copyPgn(): Promise<void> {
    await navigator.clipboard.writeText(pgn);
  }

  return (
    <>
      <Flex
        style={{ height: "100vh", width: "100vw" }}
        p={4}
        gap={4}
        direction={{ base: "column", md: "row" }}
      >
        <Box
          flex={1}
          style={{
            // backgroundColor: "red",
            padding: "2rem",
            width: "100%",
            aspectRatio: 1,
          }}
        >
          <div
            style={{ width: "min(100%, 100vw)", aspectRatio: 1 }}
            ref={groundRef}
          ></div>
        </Box>
        <Box flex={1} style={{ padding: "3rem" }}>
          <Stack>
            <div>
              <Input
                disabled
                onChange={(x) => setFen(x.target.value)}
                value={fen}
                placeholder="Enter position FEN"
              />
              {/* <Button onClick={updateSide}>I play {side}</Button> */}
              {/* <NumberInput.Root
                value={depth.toString()}
                onValueChange={(e) => setDepth(Number(e.value))}
                defaultValue="3"
                width="200px"
              >
                <NumberInput.Control />
                <NumberInput.Input />
              </NumberInput.Root> */}
              <Button onClick={copyPgn}>Copy PGN</Button>
              {output
                // .filter((x) => x.includes("bestmove"))
                .map((x) => (
                  <div key={x}>{x}</div>
                ))}
            </div>
            <p>
              Refresh the page to start a new game. (todo: play as black, and
              new game button)
            </p>
          </Stack>
        </Box>
      </Flex>
    </>
  );
}

export default App;
