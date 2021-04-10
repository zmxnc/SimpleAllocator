#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <cstdlib>
#include <utility>
#include <new>
#include <limits>
#include <cstdint>
#include <cstring>


// To avoid duplicate function definitions if the header is included in multiple files,
// ONE #include "allocator.h" needs to be preceeded by #define ALLOCATOR_IMPLEMENTATION


// If exceptions are not supported in the current build, an allocation
// failure will cause the program to abort()
// This exists to allow compilation with -fno-exceptions (clang++) or similar
#ifdef __cpp_exceptions
  #define throw_or_abort(exception) throw exception;
#else
  #define throw_or_abort(exception) abort();
#endif


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
  };

constexpr size_t Allocator_cache :: sizeof_this = sizeof(Allocator_cache) + alignof(Allocator_cache);

class Allocator_base
  {
  public:
  
  unsigned int cache_size = 2048;
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
  Object* create (Args&& ... args);
  void clear() override;
  };


using void_fn_ptr = void (*)(void*);

// Destructor wrapper for objects in the generic allocator
// This is needed because the destructor's address cannot be taken
// the way a normal function's can, because it's allowed by C++ standard
// not to follow normal function ABI. By wrapping it in a template function,
// the compiler generates the proper calling code and a function address
// that can be taken and used normally.
template <class Object>
void destructor_wrapper (void* obj)
  {
  ((Object*)obj)->~Object();
  }

// This class is used by generic allocator to mantain 
class Obj_wrapper
  {
  public:
  const uint8_t sizeof_obj;
  const void_fn_ptr destructor_ptr;

  // Requires an Object* as it's the only way to communicate
  // to the compiler the type of Obj.
  //It isn't used (it's enough to pass a nullptr cast to the Obj type)
  // and should be optimized out by the compiler
  template <class Obj, class ... Args>
  Obj_wrapper (Obj*, Args&& ... args);
  ~Obj_wrapper();

  void* obj_ptr();
  };

// This is the generic implementation
// Allows allocation of any class (as long as
// it fits in the cache_size)
// Has a slight performance penalty, as it requires to
// store a pointer to the destructor of each object
// and the size of the object
class Generic_allocator : public Allocator_base
  {
  static constexpr auto sizeof_wrapper = sizeof(Obj_wrapper) + alignof(Obj_wrapper);
  
  public:
  Generic_allocator();
  ~Generic_allocator();

  template <class Object, class ... Args>
  Object* create (Args&& ... args);
  void clear() override;
  };



template <class Object>
Allocator<Object> :: Allocator() :
  Allocator_base()
  { cache = Allocator_cache::construct (cache_size); }

template <class Object>
Allocator<Object> :: ~Allocator()
  { clear(); }

template <class Object>
template <class ... Args>
Object* Allocator<Object> :: create (Args&& ... args)
  {
  if (sizeof_obj > cache_size)
    throw_or_abort (std::bad_alloc());
  
  if (cache->cursor + sizeof_obj >= cache->end)
    cache = Allocator_cache::construct (cache_size, cache);
  
  // Placement new: allocates Object in place avoiding unnecessary memory movements
  auto tmp = new (cache->cursor) Object (std::forward<Args> (args)...);
  cache->cursor += sizeof_obj;
  return tmp;
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


template <class Object, class ... Args>
Object* Generic_allocator :: create (Args&& ... args)
  {
  auto sizeof_obj     = sizeof(Object)      + alignof(Object);
  if (sizeof_wrapper + sizeof_obj > cache_size)
    throw_or_abort (std::bad_alloc());
  
  if (cache->cursor + sizeof_wrapper + sizeof_obj >= cache->end)
    cache = Allocator_cache::construct (cache_size, cache);
  
  auto tmp = new (cache->cursor) Obj_wrapper ((Object*)nullptr, std::forward<Args> (args)...);
  cache->cursor += sizeof_wrapper + sizeof_obj;
  return (Object*)tmp->obj_ptr();
  }


template <class Obj, class ... Args>
Obj_wrapper :: Obj_wrapper (Obj*, Args&& ... args) :
  sizeof_obj (sizeof(Obj) + alignof (Obj)),
  destructor_ptr (destructor_wrapper<Obj>)
  {
  // Check that the object's size is not bigger than what our size variable allows for
  static_assert (sizeof(Obj) + alignof (Obj) <= std::numeric_limits<uint8_t>::max(), "Generic_allocator error: object exceeds maxiumum size");
  new (obj_ptr()) Obj (std::forward<Args>(args)...);
  }


// All non template functiond definitions are in this ifdef'd area.
#ifdef ALLOCATOR_IMPLEMENTATION


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


Generic_allocator :: Generic_allocator()
  { cache = Allocator_cache::construct (cache_size); }

Generic_allocator :: ~Generic_allocator()
  { clear(); }

void Generic_allocator :: clear()
  {
  // Delete all Allocator_cache instances, save for the original one
  while (true)
    {
    // Call the destructor for the allocated objects
    for (auto pos = cache->start; pos != cache->cursor;)
      {
      auto obj_wrapper = (Obj_wrapper*)pos;
      pos += sizeof_wrapper + obj_wrapper->sizeof_obj;
      obj_wrapper->~Obj_wrapper();
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
  }

Obj_wrapper :: ~Obj_wrapper()
  {
  (*destructor_ptr) (obj_ptr());
  }

void* Obj_wrapper :: obj_ptr()
  { return (char*)this + sizeof(Obj_wrapper) + alignof(Obj_wrapper); }

#endif

#endif
