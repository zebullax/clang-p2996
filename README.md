## What is this repository?

This repository is a fork of LLVM's `llvm-project` repository, created to host a fork of the
[clang](https://clang.llvm.org) compiler front-end implementing experimental support for ISO C++
proposal [P2996 ("_Reflection for C++26_")](https://wg21.link/p2996). Development primarily takes
place on the [p2996](https://github.com/bloomberg/clang-p2996/tree/p2996) branch; you can
learn more about this fork in our [project documentation](
https://github.com/bloomberg/clang-p2996/tree/p2996/P2996.md).

The Clang/P2996 fork is highly experimental; sharp edges abound and occasional crashes should be
expected. Memory usage has not been optimized and is, in many cases, wasteful. **DO NOT use this
project to build any artifacts destined for production.**
