#include "allocator.h"


template <class Object>
Allocator :: Allocator()
  {
  first = new Allocator_cache<Object, cache_size>();
  last  = first;
  }

template <class Object>
Allocator :: ~Allocator()
  {
  empty();
  }

template <class ... Args>
template <class Object>
Object& Allocator :: create (Args ... args)
  {
  if (cache->pos == cache->end)
    cache = new Allocator_cache<Object, cache_size> (cache);

  // Placement new: allocates Object in pos avoiding extra memory movement
  auto tmp = new (pos) Object (std::forward<Args> (args)...);
  pos += sizeof (Object);
  return tmp;
  }

void Allocator :: empty()
  {
  // Delete all Allocator_cache instances, save for the original one
  while (cache->prev != nullptr)
    {
    auto tmp = cache->prev;
    // Call the destructor for the allocated objects
    for (auto cursor = cache->data; cursor != cache->pos; cursor += sizeof (Object))
      cursor->~Object();
    free(cache);
    cache = tmp;
    }
  // Reset the original instance
  cache->pos = cache->data;
  }

