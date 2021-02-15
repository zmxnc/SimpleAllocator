#include "allocator.h"


template <class Object>
Allocator :: Allocator()
  { allocate_cache(); }

template <class Object>
Allocator :: ~Allocator()
  {
  empty();
  }

template <typename Object>
template <typename ... Args>
Object& Allocator :: create (Args ... args)
  {
  if (sizeof (Object) > cache_size)
    throw std::bad_alloc();
  
  if (cursor() == end)
    allocate_cache();
  
  // Placement new: allocates Object in pos avoiding extra memory movement
  auto tmp = new (cursor()) Object (std::forward<Args> (args)...);
  cursor() += sizeof (Object);
  return (Object&)*tmp;
  }

template <class Object>
void Allocator :: empty()
  {
  // Delete all Allocator_cache instances, save for the original one
  while (previous() != nullptr)
    {
    // Call the destructor for the allocated objects
    for (auto pos = start(); pos != cursor(); pos += sizeof (Object))
      pos->~Object();
    
    auto tmp = previous();
    free(cache);
    cache = tmp;
    }
  // Reset the original instance
  // The data is not modified, and Objects allocated in the first
  // cache will remain accessible (to avoid this, we could reallocate the original
  // cache as well, at a small performance cost)
  *cursor() = start();
  }


template <class Object>
void Allocator :: allocate_cache()
  {
  auto old    = cache;
  cache       = (char*) malloc (cache_size);
  *previous() = old;
  *cursor()   = start();
  *end()      = cache + cache_size;
  }

template <class Object>
char* Allocator :: start()
  { return ((char**)cache)[3]; }

template <class Object>
char* Allocator :: end()
  { return ((char**)cache)[2]; }

template <class Object>
char* Allocator :: previous()
  { return ((char**)cache)[0]; }

template <class Object>
char* Allocator :: cursor()
  { return ((char**)cache)[1]; }

