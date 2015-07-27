#ifndef _GLIBCXX_EXPERIMENTAL_MEMORY_RESOURCE
#define _GLIBCXX_EXPERIMENTAL_MEMORY_RESOURCE 1

#include <memory>
#include <new>
#include <atomic>
#include <cstddef>
#include <bits/alloc_traits.h>

namespace std {
namespace experimental {
inline namespace fundamentals_v1 {
namespace pmr {
  class memory_resource;

  template <typename _Tp>
    class polymorphic_allocator;

  template <typename _Alloc>
    class __resource_adaptor_imp;

  template <typename _Alloc>
    using resource_adaptor = __resource_adaptor_imp<
      typename allocator_traits<_Alloc>::template rebind_alloc<char>>;

  template <typename _Tp>
    struct __uses_allocator_construction_helper;

  // Global memory resources
  memory_resource* new_delete_resource() noexcept;
  memory_resource* null_memory_resource() noexcept;

  // The default memory resource
  memory_resource* get_default_resource() noexcept;
  memory_resource* set_default_resource(memory_resource* __r) noexcept;

  // Standard memory resources

  // 8.5 Class memory_resource
  class memory_resource
  {
    static constexpr size_t __max_align = alignof(max_align_t);

  public:
    virtual ~memory_resource() { }

    void*
    allocate(size_t __bytes,
	     size_t __alignment = __max_align)
    { return do_allocate(__bytes, __alignment); }

    void
    deallocate(void* __p, size_t __bytes,
	       size_t __alignment = __max_align)
    { return do_deallocate(__p, __bytes, __alignment); }

    bool
    is_equal(const memory_resource& __other) const noexcept
    { return do_is_equal(__other); }

  protected:
    virtual void*
    do_allocate(size_t __bytes, size_t __alignment) = 0;

    virtual void
    do_deallocate(void* __p, size_t __bytes, size_t __alignment) = 0;

    virtual bool
    do_is_equal(const memory_resource& __other) const noexcept = 0;
  };

  inline bool
  operator==(const memory_resource& __a,
	     const memory_resource& __b) noexcept
  { return &__a == &__b || __a.is_equal(__b); }

  inline bool
  operator!=(const memory_resource& __a,
	     const memory_resource& __b) noexcept
  { return !(__a == __b); }


  // 8.6 Class template polymorphic_allocator
  template <class _Tp>
    class polymorphic_allocator
    {
      using __uses_alloc1_ = __uses_alloc1<memory_resource*>;
      using __uses_alloc2_ = __uses_alloc2<memory_resource*>;

      template<typename _Tp1, typename... _Args>
	void
	_M_construct(__uses_alloc0, _Tp1* __p, _Args&&... __args)
	{ ::new(__p) _Tp1(std::forward<_Args>(__args)...); }

      template<typename _Tp1, typename... _Args>
	void
	_M_construct(__uses_alloc1_, _Tp1* __p, _Args&&...  __args)
	{ ::new(__p) _Tp1(allocator_arg, this->resource(),
			  std::forward<_Args>(__args)...); }

      template<typename _Tp1, typename... _Args>
	void
	_M_construct(__uses_alloc2_, _Tp1* __p, _Args&&...  __args)
	{ ::new(__p) _Tp1(std::forward<_Args>(__args)...,
			  this->resource()); }

    public:
      using value_type = _Tp;

      polymorphic_allocator() noexcept
      : _M_resource(get_default_resource())
      { }

      polymorphic_allocator(memory_resource* __r)
      : _M_resource(__r)
      { _GLIBCXX_DEBUG_ASSERT(__r); }

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

      template <typename _Tp1, typename... _Args> //used here
	void construct(_Tp1* __p, _Args&&... __args)
	{
	  auto __use_tag = __use_alloc<_Tp1, memory_resource*,
	       _Args...>(this->resource());
	  _M_construct(__use_tag, __p, std::forward<_Args>(__args)...);
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

	  ::new(__p) std::pair<_Tp1, _Tp2>(piecewise_construct,
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
	{ __p->~_Up(); }

      // Return a default-constructed allocator (no allocator propagation)
      polymorphic_allocator select_on_container_copy_construction() const
      { return polymorphic_allocator(); }

      memory_resource* resource() const
      { return _M_resource; }

    private:
      template<typename _Tuple>
	_Tuple&&
	_M_construct_p(__uses_alloc0, _Tuple& __t)
	{ return std::move(__t); }

      template<typename... _Args>
	decltype(auto)
	_M_construct_p(__uses_alloc1_ __ua, tuple<_Args...>& __t)
	{ return tuple_cat(make_tuple(allocator_arg, __ua._M_a),
			   std::move(__t)); }

      template<typename... _Args>
	decltype(auto)
	_M_construct_p(__uses_alloc2_ __ua, tuple<_Args...>& __t)
	{ return tuple_cat(std::move(__t), make_tuple(__ua._M_a)); }

      memory_resource* _M_resource;
    };

  template <class _Tp1, class _Tp2>
    bool operator==(const polymorphic_allocator<_Tp1>& __a,
		    const polymorphic_allocator<_Tp2>& __b) noexcept
    { return *__a.resource() == *__b.resource(); }

  template <class _Tp1, class _Tp2>
    bool operator!=(const polymorphic_allocator<_Tp1>& __a,
		    const polymorphic_allocator<_Tp2>& __b) noexcept
    { return !(__a == __b); }

  // 8.7.1 __resource_adaptor_imp
  template <typename _Alloc>
    class __resource_adaptor_imp : public memory_resource
    {
    public:
      using allocator_type = _Alloc;

      __resource_adaptor_imp() = default;
      __resource_adaptor_imp(const __resource_adaptor_imp&) = default;
      __resource_adaptor_imp(__resource_adaptor_imp&&) = default;

      explicit __resource_adaptor_imp(const _Alloc& __a2)
      : _M_alloc(__a2)
      { }

      explicit __resource_adaptor_imp(_Alloc&& __a2)
      : _M_alloc(std::move(__a2))
      { }

      __resource_adaptor_imp&
      operator=(const __resource_adaptor_imp&) = default;

      allocator_type get_allocator() const { return _M_alloc; }

    protected:
      virtual void*
      do_allocate(size_t __bytes, size_t __alignment)
      {
	using _Aligned_alloc = typename _Alloc::template rebind<char>::other;
	size_t __new_size = _Aligned_size(__bytes,
					  _M_supported(__alignment) ?
					  __alignment : __max_align);
	return _Aligned_alloc().allocate(__new_size);
      }

      virtual void
      do_deallocate(void* __p, size_t __bytes, size_t __alignment)
      {
	using _Aligned_alloc = typename _Alloc::template rebind<char>::other;
	size_t __new_size = _Aligned_size(__bytes,
					  _M_supported(__alignment) ?
					  __alignment : __max_align);
	_Aligned_alloc().deallocate(static_cast<typename
				    _Aligned_alloc::pointer>(__p),
				    __new_size);
      }

      virtual bool
      do_is_equal(const memory_resource& __other) const noexcept
      {
	auto __p = dynamic_cast<const __resource_adaptor_imp*>(&__other);
	return __p ? (_M_alloc == __p->_M_alloc) : false;
      }

    private:
      // Calculate Aligned Size
      // Returns a size that is larger than or equal to __size and divided by
      // __alignment, where __alignment is required to be the power of 2.
      size_t _Aligned_size(size_t __size, size_t __alignment)
      { return ((__size - 1)|(__alignment - 1)) + 1; }

      bool _M_supported (size_t __x)
      { return ((__x != 0) && (__x != 0) && !(__x & (__x - 1))); }

      _Alloc _M_alloc;
    };

  // Global memory resources
  inline std::atomic<memory_resource*>&
  _S_get_default_resource()
  {
    static std::atomic<memory_resource*>
      _S_default_resource(new_delete_resource());
    return _S_default_resource;
  }

  inline memory_resource*
  new_delete_resource() noexcept
  {
    static resource_adaptor<std::allocator<char>> __r;
    return static_cast<memory_resource*>(&__r);
  }

  template <typename _Alloc>
    class __null_memory_resource : private memory_resource
    {
    protected:
      void* do_allocate(size_t __bytes, size_t __alignment)
      { std::__throw_bad_alloc(); }

      void do_deallocate(void* __p, size_t __bytes, size_t __alignment)
      { }

      bool do_is_equal(const memory_resource& __other) const noexcept
      { }

      friend memory_resource* null_memory_resource();
    };

  inline memory_resource*
  null_memory_resource() noexcept
  {
    static __null_memory_resource<void> __r;
    return static_cast<memory_resource*>(&__r);
  }

  // The default memory resource
  inline memory_resource*
  get_default_resource() noexcept
  { return _S_get_default_resource().load(); }

  inline memory_resource*
  set_default_resource(memory_resource* __r) noexcept
  {
    if ( __r == nullptr)
      __r = new_delete_resource();
    return _S_get_default_resource().exchange(__r);
  }

}
}
}
}

#endif
