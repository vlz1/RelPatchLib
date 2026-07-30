# RelPatchLib

Cross-platform code patching/hooking library for x86/x86_64. 

Uses [Zydis](https://github.com/zyantific/zydis) for instruction decoding and disassembly.

***

## Warning

Don't use this to patch games with any sort of anti-cheat. This library makes no attempt to hide what it's doing, and it'll probably be detected immediately.

## Features

- Pattern matching and searching
  * Search for data that matches a pattern with nibble (4-bit) granularity.
  * Supports wildcards.
- Prologue hooks
  * Install hooks that run when a function is called.
  * Read and manipulate argument values.
- WIP: Epilogue hooks
  * Install hooks that run when a function returns.
  * Read and manipulate the return value.
- WIP: Hook priorities
  * Multiple hooks can be installed on the same function.
  * When the function is called/returns, the hooks are run according to their priorities (ascending order).

## Examples

TODO

## Build

TODO