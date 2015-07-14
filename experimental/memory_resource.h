#ifndef _GLIBCXX_MEMORY_RESOURCE
#define _GLIBCXX_MEMORY_RESOURCE 1

#include <memory>
#include <new>
#include <cstddef>
#include <bits/alloc_traits.h>

namespace std
{
namespace experimental
{
inline namespace fundamentals_v1
{
namespace pmr {
  // Decleartion
  class memory_resource;

  template <typename _Tp>
    class polymorphic_allocator;

  template <typename _Alloc>
    class resource_adaptor_imp;

  template <typename _Alloc>
    using resource_adaptor = resource_adaptor_imp<
      typename allocator_traits<_Alloc>::template rebind_alloc<char>>;

  // Global memory resources TODO
  memory_resource* new_delete_resource() noexcept
  { }
  memory_resource* null_memory_resource() noexcept
  { }

  // The default memory resource TODO
  memory_resource* set_default_resource(memory_resource* r) noexcept
  { }
  memory_resource* get_default_resource() noexcept
  { }

  // Standard memory resources TODO
  struct pool_options;
  class synchronized_pool_resource;
  class unsynchronized_pool_resource;
  class monotonic_buffer_resource;

  // 8.5 Class memory_resource
  class memory_resource
  {
    static constexpr size_t __max_align = alignof(max_align_t);
  public:
    virtual ~memory_resource();

    void* allocate(size_t __bytes,
		   size_t __alignment = __max_align)
    { return do_allocate(__bytes, __alignment); }

    void deallocate(void* __p, size_t __bytes,
		    size_t __alignment = __max_align)
    { return do_deallocate(__p, __bytes, __alignment); }

    bool is_equal(const memory_resource& __other) const noexcept
    { return do_is_equal(__other);}

  protected:
    virtual void* do_allocate(size_t __bytes, size_t __alignment) = 0;
    virtual void do_deallocate(void* __p, size_t __bytes,
			       size_t __alignment) = 0;
    virtual bool do_is_equal(const memory_resource& __other) const noexcept = 0;
  };

  bool operator==(const memory_resource& __a,
		  const memory_resource& __b) noexcept
  { return &__a == &__b || __a.is_equal(__b); }

  bool operator!=(const memory_resource& __a,
		  const memory_resource& __b) noexcept
  { return !(__a == __b); }

  template <typename _Tp>
    class __constructor_helper_imp
    {
      using dont_care_type = bool;

    public:
      // Construct with no allocator
      template <typename... _Args>
	__constructor_helper_imp(false_type, _Args&&... __args)
      	: _M_tp(std::forward<_Args>(__args)...) { }

      // Construct with allocator
      // 1. uses_allocator == false
      // 1.1 no pair construction - ignore allocator
      template <typename _Alloc, typename... _Args>
	__constructor_helper_imp(allocator_arg_t,
				 const _Alloc& __a,
				 false_type,
				 dont_care_type,
				 dont_care_type,
				 _Args&&... __args)
      	: _M_tp(std::forward<_Args>(__args)...) { }

      // 2. uses_allocator == true && prepend allocator argument
      template <typename _Alloc, typename... _Args>
	__constructor_helper_imp(allocator_arg_t,
				 const _Alloc& __a,
				 true_type,
				 dont_care_type,
				 true_type,
				 _Args&&... __args)
      	: _M_tp(allocator_arg, __a, std::forward<_Args>(__args)...) { }

      // 3. uses_allocator == true && append allocator argument
      template <typename _Alloc, typename... _Args>
	__constructor_helper_imp(allocator_arg_t,
				 const _Alloc& __a,
				 true_type,
				 true_type,
				 dont_care_type,
				 _Args&&... __args)
      	: _M_tp(std::forward<_Args>(__args)..., __a) { }

      // 4. ill-formed
      template <typename _Alloc, typename... _Args>
	__constructor_helper_imp(allocator_arg_t,
				 const _Alloc& __a,
				 true_type,
				 false_type,
				 false_type,
				 _Args&&... __args)
      	: _M_tp(std::forward<_Args>(__args)...)
	{
	  static_assert(sizeof(_Alloc) == 0,
			"Type uses an allocator but "
			"allocator-aware constructor "
			"is missing");
	}

    private:
      _Tp _M_tp;
    };

  template <typename _Tp>
    class __constructor_helper : private __constructor_helper_imp<_Tp>
    {
      using _Base = __constructor_helper_imp<_Tp>;

    public:
      template <typename _Alloc, typename... _Args>
	__constructor_helper(allocator_arg_t, _Alloc&& __a, _Args&&... __args)
      	: _Base(allocator_arg, __a,
      	        uses_allocator<_Tp, _Alloc>(),
      	        is_constructible<_Tp, _Args..., _Alloc>(),
      	        is_constructible<_Tp, allocator_arg_t, _Alloc, _Args...>(),
      	        std::forward<_Args>(__args)...)
      	{ }

      template <typename... _Args>
	__constructor_helper(_Args&&... __args)
	: _Base(false_type(), std::forward<_Args>(__args)...)
	{ }

    };

  // 8.6 Class template polymorphic_allocator
  template <class _Tp>
    class polymorphic_allocator
    {
      using __uses_alloc1_ = __uses_alloc1<memory_resource*>;
      using __uses_alloc2_ = __uses_alloc2<memory_resource*>;

    public:
      using value_type = _Tp;

      polymorphic_allocator() noexcept
      : _M_resource(get_default_resource())
      { }

      polymorphic_allocator(memory_resource* __r)
      : _M_resource(__r ? __r : get_default_resource())
      { }

      polymorphic_allocator(const polymorphic_allocator& __other) = default;

      template <typename _Up>
	polymorphic_allocator(const polymorphic_allocator<_Up>&
			      __other) noexcept
	: _M_resource(__other.resource())
	{ }

      polymorphic_allocator&
	operator=(const polymorphic_allocator& __rhs) = default;

      _Tp* allocate(size_t __n)
      { return static_cast<_Tp*>(_M_resource->allocate(__n * sizeof(_Tp),
						       alignof(_Tp))); }

      void deallocate(_Tp* __p, size_t __n)
      { _M_resource->deallocate(__p, __n * sizeof(_Tp), alignof(_Tp)); }

      template <typename _Tp1, typename... _Args>
	void construct(_Tp1* __p, _Args&&... __args)
	{
	  using _Ctor_imp = __constructor_helper<_Tp1>;
	  ::new ((void*)__p) _Ctor_imp(allocator_arg,
				       this->resource(),
				       std::forward<_Args>(__args)...);
	}

      // Specializations for pair using piecewise construction
      template <typename _Tp1, typename _Tp2,
	       typename... _Args1, typename... _Args2>
	void construct(pair<_Tp1, _Tp2>* __p, piecewise_construct_t,
		       tuple<_Args1...> __x,
		       tuple<_Args2...> __y)
	{
	  auto __x_use_tag =
	    __use_alloc<_Tp1, memory_resource*, _Args1...>(this->resource());
	  auto __y_use_tag =
	    __use_alloc<_Tp2, memory_resource*, _Args2...>(this->resource());

	  using _Ctor_imp = __constructor_helper<pair<_Tp1, _Tp2>>;
	  ::new ((void*)__p) _Ctor_imp(piecewise_construct,
				       _M_construct_p(__x_use_tag, __x),
				       _M_construct_p(__y_use_tag, __y));
	}

      template <typename _Tp1, typename _Tp2>
	void construct(pair<_Tp1,_Tp2>* __p)
	{ this->construct(__p, piecewise_construct, tuple<>(), tuple<>()); }

      template <typename _Tp1, typename _Tp2, typename _Up, typename _Vp>
	void construct(pair<_Tp1,_Tp2>* __p, _Up&& __x, _Vp&& __y)
	{ this->construct(__p, piecewise_construct,
			  forward_as_tuple(std::forward<_Up>(__x)),
			  forward_as_tuple(std::forward<_Vp>(__y))); }

      template <typename _Tp1, typename _Tp2, typename _Up, typename _Vp>
	void construct(pair<_Tp1,_Tp2>* __p, const std::pair<_Up, _Vp>& __pr)
	{ this->construct(__p, piecewise_construct, forward_as_tuple(__pr.first),
			  forward_as_tuple(__pr.second)); }

      template <typename _Tp1, typename _Tp2, typename _Up, typename _Vp>
	void construct(pair<_Tp1,_Tp2>* __p, pair<_Up, _Vp>&& __pr)
	{ this->construct(__p, piecewise_construct,
			  forward_as_tuple(std::forward<_Up>(__pr.first)),
			  forward_as_tuple(std::forward<_Vp>(__pr.second))); }

      template <typename _Up>
	void destroy(_Up* __p)
	{ __p->~T(); }

      // Return a default-constructed allocator (no allocator propagation)
      polymorphic_allocator select_on_container_copy_construction() const
      { return polymorphic_allocator(); }

      memory_resource* resource() const
      { return _M_resource; }

    private:
      memory_resource* _M_resource;

      template<typename _Tuple>
	_Tuple&&
	_M_construct_p(__uses_alloc0, _Tuple& __t)
	{ return std::move(__t); }

      template<typename... _Args>
	decltype(auto)
	_M_construct_p(__uses_alloc1_, tuple<_Args...>& __t)
	{ return tuple_cat(make_tuple(allocator_arg, this->resources()),
			   std::move(__t)); }

      template<typename... _Args>
	decltype(auto)
	_M_construct_p(__uses_alloc2_, tuple<_Args...>& __t)
	{ return tuple_cat(std::move(__t), make_tuple(this->resources())); }
    };

  template <class _Tp1, class _Tp2>
    bool operator==(const polymorphic_allocator<_Tp1>& __a,
		    const polymorphic_allocator<_Tp2>& __b) noexcept
    { return *__a.resource() == *__b.resource(); }

  template <class _Tp1, class _Tp2>
    bool operator!=(const polymorphic_allocator<_Tp1>& __a,
		    const polymorphic_allocator<_Tp2>& __b) noexcept
    { return ! (__a == __b); }

  // 8.7.1 resource_adaptor_imp
  template <typename _Alloc>
    class resource_adaptor_imp : public memory_resource
    {
    public:
      using allocator_type = _Alloc;

      resource_adaptor_imp() = default;
      resource_adaptor_imp(const resource_adaptor_imp&) = default;
      resource_adaptor_imp(resource_adaptor_imp&&) = default;

      explicit resource_adaptor_imp(const _Alloc& __a2)
      : _M_alloc(__a2)
      { }

      explicit resource_adaptor_imp(_Alloc&& __a2)
      : _M_alloc(std::move(__a2))
      { }

      resource_adaptor_imp& operator=(const resource_adaptor_imp&) = default;

      allocator_type get_allocator() const { return _M_alloc; }



    private:
      _Alloc _M_alloc;
    };

}
}
}
}

#endif
