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
  -sEXPORTED_RUNTIME_METHODS=ccall,cwrap\
  -o quokka.js \
  -s ASSERTIONS
```
