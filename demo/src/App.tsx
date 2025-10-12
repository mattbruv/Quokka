import { useEffect, useState } from "react";
//@ts-expect-error no types
import createQuokka from "./quokka.js";

function App() {
  // Hi mom

  const [output, setOutput] = useState<string[]>([]);

  async function calculate() {
    const depth = 4;
    const module = await createQuokka();
    // takes a depth and FEN, returns program output
    const int_sqrt = module.cwrap("int_sqrt", "string", ["number"]);
    console.log(int_sqrt(depth));
  }

  return (
    <>
      <div>
        <h1>hi Quokka</h1>
        <button onClick={calculate}>calculate</button>
        {output.map((x) => (
          <div>{x}</div>
        ))}
      </div>
    </>
  );
}

export default App;
