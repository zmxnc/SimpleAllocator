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

  template <class ... Args>
  Object& create (Args ... args);
  void clear();

  private:
  
  void allocate_cache();
  // Start of the memory available for allocations
  char*& start();
  // End of the memory available for allocations
  char*& end();
  // Address of the previous cache
  char*& previous();
  // Position of the curson in the current cache
  char*& cursor();
  };


template <class Object>
Allocator<Object> :: Allocator() :
  cache (nullptr)
  { allocate_cache(); }

template <class Object>
Allocator<Object> :: ~Allocator()
  { clear(); }

template <class Object>
template <class ... Args>
Object& Allocator<Object> :: create (Args ... args)
  {
  if (sizeof (Object) > cache_size)
    throw std::bad_alloc();
  
  if (cursor() == end())
    allocate_cache();
  
  // Placement new: allocates Object in pos avoiding extra memory movement
  auto tmp = new (cursor()) Object (std::forward<Args> (args)...);
  cursor() += sizeof (Object);
  return (Object&)*tmp;
  }

template <class Object>
void Allocator<Object> :: clear()
  {
  // Delete all Allocator_cache instances, save for the original one
  while (true)
    {
    // Call the destructor for the allocated objects
    for (char* pos = (char*)&start(); pos != cursor(); pos += sizeof (Object))
      ((Object*)pos)->~Object();

    if (previous() == nullptr)
      break;
    else
      {
      auto tmp = previous();
      free(cache);
      cache = tmp;
      }
    }
  // Reset the original instance
  // The data is not modified, and Objects allocated in the first
  // cache will remain accessible (to avoid this, we could reallocate the original
  // cache as well, at a small performance cost)
  cursor() = (char*)&start();
  }

template <class Object>
void Allocator<Object> :: allocate_cache()
  {
  auto old    = cache;
  cache       = (char*) malloc (cache_size);
  previous()  = old;
  cursor()    = (char*)&start();
  end()       = cache + cache_size;
  }

template <class Object>
char*& Allocator<Object> :: start()
  { return *((char**)cache + 3); }

template <class Object>
char*& Allocator<Object> :: end()
  { return *((char**)cache + 2); }

template <class Object>
char*& Allocator<Object> :: previous()
  { return *((char**)cache + 0); }

template <class Object>
char*& Allocator<Object> :: cursor()
  { return *((char**)cache + 1); }


#endif
