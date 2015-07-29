#ifndef MY_DEBUG_DOT_H
#define MY_DEBUG_DOT_H 1

#include <cassert>
#include <iostream>
using std::cout;
using std::endl;
#define VERIFY(X) assert(X)
#define DEBUG(X) cout<<X<<endl

#endif
