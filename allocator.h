
#ifndef ALLOCATOR_H
#define ALLOCATOR_h



template <class Object>
class Allocator
  {
  Allocator_cache *cache;
  
  public:
  unsigned int cache_size = 64;
  
  Allocator();
  ~Allocator();

  template <class ... Args>
  Object& create (Args ... args);
  void empty();
  };

template <class Object, int size>
class Allocator_cache
  {
  Allocator_cache* prev;
  Object data[size], *pos, *end;

  public:
  Allocator_cache (Allocator_cache* prev = nullptr) :
    prev (prev),
    pos (data),
    end (data + size * sizeof(Object))
    {  }
  ~Allocator_cache() = default;
  };

#endif
