```
emcc ../../src/*.cpp \
  -O0 \
  -g \
  -s WASM=1 \
  -s MODULARIZE=1 \
  -s EXPORT_ES6=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s INVOKE_RUN=0 \
  -s EXIT_RUNTIME=0 \
  -s FORCE_FILESYSTEM=1 \
  -s EXPORTED_FUNCTIONS=_int_sqrt \
  -s STACK_SIZE=8MB \
  -sEXPORTED_RUNTIME_METHODS=ccall,cwrap\
  -o quokka.js \
  -s ASSERTIONS
```

https://emscripten.org/docs/porting/connecting_cpp_and_javascript/Interacting-with-code.html#interacting-with-code-ccall-cwrap

Lots of memory errors, fixed this by vibe-debugging with chatgpt and passing it console log errors at runtime, with debug symbols

STACK_SIZE=8MB is needed because WASM was crashing because I was assigning structs and over-using the alloted stack size in helper functions
