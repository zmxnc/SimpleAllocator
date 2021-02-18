#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <cstdlib>
#include <utility>
#include <new>


class Allocator_base
  {
  public:
  
  unsigned int cache_size = 2048;
  Allocator_base() = default;
  virtual ~Allocator_base() = 0;

  virtual void clear() = 0;

  protected:
  
  // The data cache currently in use
  char *cache;
  
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


// This class contains the homogeneous implementation
// (allows for a single data class)
template <class Object>
class Allocator : public Allocator_base
  {
  public:
  Allocator();
  ~Allocator();

  template <class ... Args>
  Object& create (Args ... args);
  void clear() override;
  };


// This is the generic implementation
// Allows allocation of any class (as long as
// it fits in the cache_size)
// Has a slight performance penalty, as it requires to
// store a pointer to the destructor of each object
class Generic_allocator : public Allocator_base
  {
  public:
  Generic_allocator();
  ~Generic_allocator();

  template <class Object, class ... Args>
  Object& create (Args ... args);
  void clear() override;
  };


Allocator_base :: ~Allocator_base()
  {  }

void Allocator_base :: allocate_cache()
  {
  auto old    = cache;
  cache       = (char*) malloc (cache_size);
  previous()  = old;
  cursor()    = (char*)&start();
  end()       = cache + cache_size;
  }

char*& Allocator_base :: start()
  { return *((char**)cache + 3); }

char*& Allocator_base :: end()
  { return *((char**)cache + 2); }

char*& Allocator_base :: previous()
  { return *((char**)cache + 0); }

char*& Allocator_base :: cursor()
  { return *((char**)cache + 1); }



template <class Object>
Allocator<Object> :: Allocator() :
  Allocator_base()
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


#endif
