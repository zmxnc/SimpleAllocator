#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <cstdlib>
#include <utility>
#include <new>
#include <limits>
#include <cstdint>
#include <cstring>


class Allocator_base
  {
  public:
  
  unsigned int cache_size = 2048;
  Allocator_base() = default;
  virtual ~Allocator_base() = 0;

  virtual void clear() = 0;

  protected:
  
  // The data cache currently in use
  void *cache;
  
  void allocate_cache();
  // Start of the memory available for allocations
  void*& start();
  // End of the memory available for allocations
  void*& end();
  // Address of the previous cache
  void*& previous();
  // Position of the curson in the current cache
  void*& cursor();
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
// and the size of the object
class Generic_allocator : public Allocator_base
  {
  public:
  Generic_allocator();
  ~Generic_allocator();

  template <class Object, class ... Args>
  Object& create (Args ... args);
  void clear() override;
  };


// Wrapper class for objects in the generic allocator.
// This is needed because the destructor's address cannot be taken
// the way a normal function's can, because it's allowed by C++ standard
// not to follow normal function ABI. By wrapping it in a template function,
// the compiler generates the proper calling code and a member function address
// that can be taken and used normally.
template <class Object>
class Obj_wrapper
  {
  Object obj;
  
  public:
  template <class ... Args>
  Obj_wrapper (Args ... args) :
    Object (std::forward(args)...)
    {  }

  void destruct()
    { obj.~Object(); }

  private:

  // Destructor should never be called, the Object can only be destructed by calling
  // Obj_wrapper::destruct()
  ~Obj_wrapper() = default;
  };



Allocator_base :: ~Allocator_base()
  {  }

void Allocator_base :: allocate_cache()
  {
  auto old    = cache;
  cache       = malloc (cache_size);
  previous()  = old;
  cursor()    = &start();
  end()       = cache + cache_size;
  }

void*& Allocator_base :: start()
  { return *((void**)cache + 3); }

void*& Allocator_base :: end()
  { return *((void**)cache + 2); }

void*& Allocator_base :: previous()
  { return *((void**)cache + 0); }

void*& Allocator_base :: cursor()
  { return *((void**)cache + 1); }



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
  
  // Placement new: allocates Object in place avoiding unnecessary memory movements
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
  // cache will remain accessible (to avoid this, we could reallocate or
  // overwrite the original cache as well, at a small performance cost)
  cursor() = (char*)&start();
  }


Generic_allocator :: Generic_allocator()
  { allocate_cache(); }
Generic_allocator :: ~Generic_allocator()
  { clear(); }

template <class Object, class ... Args>
Object& Generic_allocator :: create (Args ... args)
  {
  // Member function pointers can have variable size ->
  // it needs to be stored before the mem_fn_ptr
  // to retrieve the object start position at deallocation time
  const uint8_t destructor_ptr_size = sizeof(&Obj_wrapper<Object>::destruct);
  const uint8_t wrapper_size        = sizeof(Obj_wrapper<Object>) + alignof(Obj_wrapper<Object>);
  if (sizeof(short) + destructor_ptr_size + wrapper_size > cache_size)
    throw std::bad_alloc();
  
  if (cursor() == end())
    allocate_cache();

  static_assert (wrapper_size        <= std::numeric_limits<uint8_t>::max());
  static_assert (destructor_ptr_size <= std::numeric_limits<uint8_t>::max());
  
  // Store the fn pointer size
  memcpy (destructor_ptr_size, cursor(), sizeof(uint8_t));
  cursor() += sizeof(uint8_t);
  // Store the object size
  memcpy (wrapper_size, cursor(), sizeof(uint8_t));
  cursor() += sizeof(uint8_t);
  // Store the fn pointer right after
  memcpy (&Obj_wrapper<Object>::destruct, cursor(), destructor_ptr_size);
  cursor() += destructor_ptr_size;
  // Placement new: allocates Object in place avoiding unnecessary memory movements
  auto tmp = new (cursor()) Object (std::forward<Args> (args)...);
  cursor() += wrapper_size;
  return (Object&)*tmp;
  }

void Generic_allocator :: clear()
  {
  // Delete all Allocator_cache instances, save for the original one
  while (true)
    {
    // Call the destructor for the allocated objects
    for (char* pos = (char*)&start(); pos != cursor();)
      {
      auto destructor_ptr_size = (uint8_t)*pos;
      pos += sizeof(uint8_t);
      auto wrapper_size = (uint8_t)*pos;
      pos += sizeof(uint8_t);
      void *destructor_ptr = pos;
      pos += destructor_ptr_size;
      auto wrapper_ptr = pos;
      pos += wrapper_size;
      (*destructor_ptr) (wrapper_ptr);
      }

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
  // cache will remain accessible (to avoid this, we could reallocate or
  // overwrite the original cache as well, at a small performance cost)
  cursor() = (char*)&start();
  }

#endif
