import { useEffect } from "react";
//@ts-expect-error no types
import createQuokka from "./quokka.js";

function App() {
  // Hi mom

  useEffect(() => {
    createQuokka().then((module: any) => {
      console.log(module);
      const int_sqrt = module.cwrap("int_sqrt", "number", ["number"]);
      console.log(int_sqrt(120));
      int_sqrt(28);
    });
  }, []);

  return (
    <>
      <div>
        <h1>hi Quokka</h1>
      </div>
    </>
  );
}

export default App;
