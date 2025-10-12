import { useEffect, useState } from "react";
//@ts-expect-error no types
import createQuokka from "./quokka.js";

function App() {
  // Hi mom

  const [output, setOutput] = useState<string[]>([]);
  const [fen, setFen] = useState<string>(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
  );

  async function calculate() {
    const depth = 4;
    const module = await createQuokka();
    // takes a depth and FEN, returns program output
    const int_sqrt = module.cwrap("int_sqrt", "string", ["number", "string"]);
    const stdout: string = int_sqrt(depth, fen);
    setOutput(stdout.split("\n"));
  }

  return (
    <>
      <div>
        <h1>hi Quokka</h1>
        <button onClick={calculate}>calculate</button>
        {output.map((x) => (
          <div key={x}>{x}</div>
        ))}
      </div>
    </>
  );
}

export default App;
