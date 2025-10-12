import { useEffect } from "react";
//@ts-expect-error no types
import createQuokka from "./quokka.js";

function App() {
  // Hi mom

  useEffect(() => {
    createQuokka().then((module: any) => {
      console.log(module);
      const int_sqrt = module.cwrap("int_sqrt", "string", ["number"]);
      console.log(int_sqrt(2));
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
