// { dg-options "-std=gnu++1y" }

// Copyright (C) 2015 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

// 8.2.1 Class template shared_ptr [memory.smartptr.shared]

#include <experimental/memory>
#include <testsuite_hooks.h>

struct A
{
    virtual ~A() { }
};

struct B : A
{
};

// 20.8.2.2.7 shared_ptr comparison
int
test01()
{
  bool test __attribute__((unused)) = true;

  // test empty shared_ptrs compare equivalent
  std::experimental::shared_ptr<A[10]> p1;
  std::experimental::shared_ptr<B[10]> p2;
  VERIFY( p1 == p2 );
  VERIFY( !(p1 != p2) );
  VERIFY( !(p1 < p2) && !(p2 < p1) );
  return 0;
}

int
test02()
{
  bool test __attribute__((unused)) = true;

  std::experimental::shared_ptr<A[10]> A_default;

  std::experimental::shared_ptr<A[10]> A_from_A(new A[10]);
  VERIFY( A_default != A_from_A );
  VERIFY( !(A_default == A_from_A) );
  VERIFY( (A_default < A_from_A) || (A_from_A < A_default) );

  std::experimental::shared_ptr<B[10]> B_from_B(new B[10]);
  VERIFY( B_from_B != A_from_A );
  VERIFY( !(B_from_B == A_from_A) );
  VERIFY( (B_from_B < A_from_A) || (A_from_A < B_from_B) );

  A_from_A.reset();
  VERIFY( A_default == A_from_A );
  VERIFY( !(A_default != A_from_A) );
  VERIFY( !(A_default < A_from_A) && !(A_from_A < A_default));

  B_from_B.reset();
  VERIFY( B_from_B == A_from_A );
  VERIFY( !(B_from_B != A_from_A) );
  VERIFY( !(B_from_B < A_from_A) && !(A_from_A < B_from_B) );

  return 0;
}

int
main()
{
  return 0;
}
