#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <cstdlib>
#include <utility>
#include <new>


template <class Object>
class Allocator
  {
  // The data cache currently in use
  char *cache;
  
  public:
  unsigned int cache_size = 2048;
  
  Allocator();
  ~Allocator();

  template <typename ... Args>
  Object& create (Args ... args);
  void empty();

  private:
  
  void allocate_cache();
  // Start of the memory available for allocations
  char* start();
  // End of the memory available for allocations
  char* end();
  // Address of the previous cache
  char* previous();
  // Position of the curson in the current cache
  char* cursor();
  };

#endif
