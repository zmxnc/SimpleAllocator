#include <iostream>
#include <assert.h>
#include <string>

#define ALLOCATOR_IMPLEMENTATION
#include "allocator.h"

using namespace std;


class TestObj
  {
  public:
  inline static int counter = 0;
  int id;
  TestObj() :
    id (++counter)
    {  }
  ~TestObj()
    { counter--; }
  };

int main()
{
  // Test basic functionality on int
  {
  Allocator<int> allocator;
  auto& a = allocator.create (1);
  auto& b = allocator.create (2);
  auto& c = allocator.create (5);
  c = a + b;

  assert (a == 1);
  assert (b == 2);
  assert (c == (a+b));
  allocator.clear();
  cerr << "Basic test :             OK\n";
  }

  // Test that destructors are working properly
  {
  Allocator<TestObj> allocator;
  auto& a = allocator.create();
  auto& b = allocator.create();

  assert (a.id == 1);
  assert (b.id == 2);
  assert (TestObj::counter == 2);
  allocator.clear();
  assert (TestObj::counter == 0);
  cerr << "Destructor test :        OK\n";
  }

  // Test Generic_allocator
  {
  Generic_allocator allocator;
  auto& a = allocator.create<TestObj>();
  auto& b = allocator.create<int>(213123);
  auto& c = allocator.create<TestObj>();

  assert (a.id == 1);
  assert (b == 213123);
  assert (c.id == 2);
  assert (TestObj::counter == 2);
  allocator.clear();
  assert (TestObj::counter == 0);
  cerr << "Generic_allocator test : OK\n";
  }
  
  return 0;
}
