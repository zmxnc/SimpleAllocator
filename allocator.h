#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <cstdlib>
#include <utility>
#include <new>
#include <limits>
#include <cstdint>
#include <cstring>


// Handles constructing/destruction our cache,
// maintaining the addresses needed by the allocator
class Allocator_cache
  {
  public:

  static Allocator_cache* construct (size_t, Allocator_cache* = nullptr);
  static void destruct (Allocator_cache*);
  
  // Start of the memory available for allocations
  char *start;
  // End of the memory available for allocations
  void *end;
  // Address of the previous cache
  Allocator_cache *previous;
  // Position of the curson in the current cache
  char *cursor;

  private:

  // Really should be constexpr, but can't since it needs to be defined inline,
  // and also after the class is fully defined
  static const size_t sizeof_this;
  
  // Hidden constructor: allocation should only be handled by ::construct()
  Allocator_cache (char*, size_t, Allocator_cache*);
  Allocator_cache() = default;
  };

constexpr size_t Allocator_cache :: sizeof_this = sizeof(Allocator_cache) + alignof(Allocator_cache);

class Allocator_base
  {
  public:
  
  unsigned int cache_size = 2048;
  Allocator_base() = default;
  virtual ~Allocator_base() = 0;

  virtual void clear() = 0;

  protected:
  
  // The data cache currently in use
  Allocator_cache *cache;
  };


// This class contains the homogeneous implementation
// (allows for a single data class)
template <class Object>
class Allocator : public Allocator_base
  {
  static constexpr auto sizeof_obj = sizeof(Object) + alignof(Object);
  
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



Allocator_cache* Allocator_cache :: construct (size_t sizeof_cache, Allocator_cache* old)
  {
  auto addr = (char*) malloc (sizeof_cache + sizeof_this);
  
  return (Allocator_cache*) new (addr) Allocator_cache (addr, sizeof_cache, old);
  }

void Allocator_cache :: destruct (Allocator_cache* cache)
  { free (cache); }

Allocator_cache :: Allocator_cache (char *addr, size_t sizeof_cache, Allocator_cache *old) :
  start (addr + sizeof_this),
  end (start + sizeof_cache),
  previous (old),
  cursor (start)
  {  }


Allocator_base :: ~Allocator_base()
  {  }


template <class Object>
Allocator<Object> :: Allocator() :
  Allocator_base()
  { cache = Allocator_cache::construct (cache_size); }

template <class Object>
Allocator<Object> :: ~Allocator()
  { clear(); }

template <class Object>
template <class ... Args>
Object& Allocator<Object> :: create (Args ... args)
  {
  if (sizeof (Object) > cache_size)
    throw std::bad_alloc();
  
  if (cache->cursor == cache->end)
    cache = Allocator_cache::construct (cache_size, cache);
  
  // Placement new: allocates Object in place avoiding unnecessary memory movements
  auto tmp = new (cache->cursor) Object (std::forward<Args> (args)...);
  cache->cursor += sizeof_obj;
  return (Object&)*tmp;
  }

template <class Object>
void Allocator<Object> :: clear()
  {
  // Delete all Allocator_cache instances, save for the original one
  while (true)
    {
    // Call the destructor for the allocated objects
    for (auto pos = cache->start; pos != cache->cursor; pos += sizeof_obj)
      ((Object*)pos)->~Object();

    if (cache->previous == nullptr)
      break;
    else
      {
      auto tmp = cache->previous;
      Allocator_cache::destruct (cache);
      cache = tmp;
      }
    }
  // Reset the original instance
  // The data is not modified, and Objects allocated in the first
  // cache will remain accessible (to avoid this, we could reallocate or
  // overwrite the original cache as well, at a small performance cost)
  cache->cursor = cache->start;
  }

/*
Generic_allocator :: Generic_allocator()
  { cache = Allocator_cache::construct (cache_size); }
Generic_allocator :: ~Generic_allocator()
  { clear(); }

template <class Object, class ... Args>
Object& Generic_allocator :: create (Args ... args)
  {
  // Member function pointers can have variable size ->
  // it needs to be stored before the mem_fn_ptr
  // to retrieve the object start position at deallocation time
  constexpr uint8_t destructor_ptr_size = sizeof(&Obj_wrapper<Object>::destruct);
  constexpr uint8_t wrapper_size        = sizeof(Obj_wrapper<Object>) + alignof(Obj_wrapper<Object>);
  if (sizeof(short) + destructor_ptr_size + wrapper_size > cache_size)
    throw std::bad_alloc();
  
  if (cache->cursor == cache->end)
    cache = Allocator_cache::construct (cache_size, cache);

  // Check that the objects are not too big to break our
  // internal cache addressing
  static_assert (wrapper_size        <= std::numeric_limits<uint8_t>::max());
  static_assert (destructor_ptr_size <= std::numeric_limits<uint8_t>::max());

  auto destructor_ptr = &Obj_wrapper<Object>::destruct;
  
  // Store the fn pointer size
  memcpy (cache->cursor, &destructor_ptr_size, sizeof(uint8_t));
  cache->cursor += sizeof(uint8_t);
  // Store the object size
  memcpy (cache->cursor, &wrapper_size,        sizeof(uint8_t));
  cache->cursor += sizeof(uint8_t);
  // Store the fn pointer right after
  memcpy (cache->cursor, &destructor_ptr,      destructor_ptr_size);
  cache->cursor += destructor_ptr_size;
  // Placement new: allocates Object in place avoiding unnecessary memory movements
  auto tmp = new (cache->cursor) Object (std::forward<Args> (args)...);
  cache->cursor += wrapper_size;
  return (Object&)*tmp;
  }

void Generic_allocator :: clear()
  {
  // Delete all Allocator_cache instances, save for the original one
  while (true)
    {
    // Call the destructor for the allocated objects
    for (auto pos = cache->start; pos != cache->cursor;)
      {
      auto destructor_ptr_size = (uint8_t)*pos;
      pos += sizeof(uint8_t);
      auto wrapper_size = (uint8_t)*pos;
      pos += sizeof(uint8_t);
      void (*destructor_ptr) = pos;
      pos += destructor_ptr_size;
      auto wrapper_ptr = pos;
      pos += wrapper_size;
      (*destructor_ptr) (wrapper_ptr);
      }

    if (cache->previous == nullptr)
      break;
    else
      {
      auto tmp = cache->previous;
      Allocator_cache::destruct (cache);
      cache = tmp;
      }
    }
  // Reset the original instance
  // The data is not modified, and Objects allocated in the first
  // cache will remain accessible (to avoid this, we could reallocate or
  // overwrite the original cache as well, at a small performance cost)
  cache->cursor = cache->start;
  }*/

#endif
