// { dg-options "-std=gnu++11" }

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

#include <memory>
#include <experimental/memory_resource>
#include <experimental/utility>
#include <bits/uses_allocator.h>
//#include <testsuite_hooks.h>
//#include <testsuite_allocator.h>
#include "../debug.h"

using std::experimental::pmr::polymorphic_allocator;
using std::experimental::pmr::memory_resource;
using std::experimental::pmr::new_delete_resource;
using std::experimental::pmr::get_default_resource;
using std::experimental::pmr::set_default_resource;
using std::allocator_arg_t;

enum CtorType { Default, Copy, Move, Other, Tuple };

// type that takes a memory_resource before other ctor args
struct A
{
  using allocator_type = std::experimental::erased_type;

  CtorType type;
  memory_resource* alloc = nullptr;

  A() : type(Default) { }
  A(allocator_arg_t, memory_resource* a) : type(Default), alloc(a) { }
  A(const A&) : type(Copy) { }
  A(allocator_arg_t, memory_resource* a, const A&) : type(Copy), alloc(a) { }
  A(A&&) : type (Move) { }
  A(allocator_arg_t, memory_resource* a, A&&) : type (Move), alloc(a) { }
  A(int) : type(Other) { }
  A(allocator_arg_t, memory_resource* a, int) : type(Other), alloc(a) { }
};

// type that takes a memory_resource after other ctor args
struct B
{
  using allocator_type = std::experimental::erased_type;

  CtorType type;
  memory_resource* alloc = nullptr;

  B() : type(Default) { }
  B(memory_resource* a) : type(Default), alloc(a) { }
  B(const B&) : type(Copy) { }
  B(const B&, memory_resource* a) : type(Copy), alloc(a) { }
  B(B&&) : type (Move) { }
  B(B&&, memory_resource* a) : type(Move), alloc(a) { }
  B(int) : type(Other) { }
  B(int, memory_resource* a) : type(Other), alloc(a) { }
};

// type that takes no memory_resource
struct C
{
  CtorType type;
  C() : type(Default) { }
  C(const C&) : type(Copy) { }
  C(C&&) : type(Move) { }
  C(int) : type(Other) { }
};

// type that used to test piecewise_construct
struct D
{
  int piecewise_type;
  D(std::tuple<A, B>) : piecewise_type(0) { }
  D(std::tuple<C, C>) : piecewise_type(1) { }
  D(const C&, const C&) : piecewise_type(2) { }
  D(const A&, const B&) : piecewise_type(3) { }
  D() : piecewise_type(4) { }
};

// test construct for type that take
// memory_resource before other ctor args
void test01() {
  polymorphic_allocator<A> pa;
  A* p = pa.allocate(1);
  A a;

  pa.construct(p);
  VERIFY(p->alloc == get_default_resource());
  VERIFY(p->type == Default);
  pa.destroy(p);

  pa.construct(p, a);
  VERIFY(p->type == Copy);
  pa.destroy(p);

  pa.construct(p, A());
  VERIFY(p->type == Move);
  pa.destroy(p);

  pa.construct(p, 1);
  VERIFY(p->type == Other);
  pa.destroy(p);

  pa.deallocate(p, 1);
}

// test construct for type that memory_resource
// after other ctor args
void test02() {
  polymorphic_allocator<B> pa;
  B* p = pa.allocate(1);
  B b;

  pa.construct(p);
  VERIFY(p->alloc == get_default_resource());
  VERIFY(p->type == Default);
  pa.destroy(p);

  pa.construct(p, b);
  VERIFY(p->type == Copy);
  pa.destroy(p);

  pa.construct(p, B());
  VERIFY(p->type == Move);
  pa.destroy(p);

  pa.construct(p, 1);
  VERIFY(p->type == Other);
  pa.destroy(p);

  pa.deallocate(p, 1);
}

// test construct for type that not using allocator
void test03() {
  polymorphic_allocator<C> pa;
  C* p = pa.allocate(1);
  C b;

  pa.construct(p);
  VERIFY(p->type == Default);
  pa.destroy(p);

  pa.construct(p, b);
  VERIFY(p->type == Copy);
  pa.destroy(p);

  pa.construct(p, C());
  VERIFY(p->type == Move);
  pa.destroy(p);

  pa.construct(p, 1);
  VERIFY(p->type == Other);
  pa.destroy(p);

  pa.deallocate(p, 1);
}

// test piecewise_construct
void test04() {
  polymorphic_allocator<std::pair<D, D>> pa;
  std::pair<D, D>* p = pa.allocate(1);
  std::tuple<A, B> t1 = std::make_tuple(A(), B());
  std::tuple<C, C> t2 = std::make_tuple(C(), C());

  pa.construct(p, std::piecewise_construct, t2, t2);
  VERIFY(p->first.piecewise_type == 2);
  VERIFY(p->second.piecewise_type == 2);
  pa.destroy(p);

  pa.construct(p, std::piecewise_construct, t1, t1);
  VERIFY(p->first.piecewise_type == 3);
  VERIFY(p->second.piecewise_type == 3);
  pa.destroy(p);

  pa.deallocate(p, 1);
}

void test05() {
  polymorphic_allocator<std::pair<D, D>> pa;
  std::pair<D, D>* p = pa.allocate(1);

  pa.construct(p);
  VERIFY(p->first.piecewise_type == 4);
  VERIFY(p->second.piecewise_type == 4);
  pa.destroy(p);

  // TODO need thinking
  pa.construct(p, D(), D());
  VERIFY(p->first.piecewise_type == 4);
  VERIFY(p->second.piecewise_type == 4);
  pa.destroy(p);

  // TODO
  pa.construct(p, *p);
  VERIFY(p->first.piecewise_type == 4);
  VERIFY(p->second.piecewise_type == 4);
  pa.destroy(p);

  // TODO
  pa.construct(p, std::move(*p));
  VERIFY(p->first.piecewise_type == 4);
  VERIFY(p->second.piecewise_type == 4);
  pa.destroy(p);
  pa.deallocate(p, 1);
}

int main() {
  test01();
  test02();
  test03();
  test04();
  test05();
}
