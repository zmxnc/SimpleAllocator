## Simple Allocator

*Alessio Larcher 2021*

This allocator class is designed to improve the management of objects that would otherwise require frequent memory allocations, reducing the number of allocations needed and improving memory locality.
It is designed to work with workloads that require frequent single allocations, and deallocate everything at once.