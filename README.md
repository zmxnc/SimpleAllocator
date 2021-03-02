## Simple Allocator

*Alessio Larcher 2021*

This simple template libary is designed to simpify memory management in programs that want to **allocate objects very often**, and **release all at once**.

By using this allocator, instead of allocating individually, manually of through smart pointers, you can improve the memory performance of your program, by reducing heap allocations and improving the memory locality/reducing cache misses in your program.

Pros:
* Improve memory performance
* Correctly handle object destruction

Cons:
* Free everything at once (manually or at the end of the allocator's lifetime)
