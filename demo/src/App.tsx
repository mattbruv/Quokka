import { useState } from "react";
//@ts-expect-error no types
import createQuokka from "./quokka.js";
import { Button, Input, NumberInput } from "@chakra-ui/react";

function App() {
  const [output, setOutput] = useState<string[]>([]);
  const [fen, setFen] = useState<string>(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
  );
  const [depth, setDepth] = useState<number>(5);

  async function calculate() {
    const module = await createQuokka();
    // takes a depth and FEN, returns program output
    const int_sqrt = module.cwrap("int_sqrt", "string", ["number", "string"]);
    const stdout: string = int_sqrt(depth, fen);
    setOutput(stdout.split("\n"));
  }

  return (
    <>
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
    </>
  );
}

export default App;
