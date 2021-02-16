#include <iostream>

#include "allocator.h"

using namespace std;

int main()
{
  Allocator<int> allocator;
  auto& a = allocator.create (1);
  auto& b = allocator.create (2);
  auto& c = allocator.create (5);
  c = a + b;
  cerr << a << " + " << b << " = " << c << endl;
  cerr << (long int)&a << " + " << (long int)&b << " = " << (long int)&c << endl;
  allocator.clear();
  return 0;
}
