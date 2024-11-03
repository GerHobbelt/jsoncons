### Allocators

A `basic_json` has an `Allocator` template parameter and an `allocator_type` member that indicates
that it is allocator aware. `Allocator` must be a Scoped Allocator, that is, an allocator 
that applies not only to a `basic_json`'s data member, but also to its data member's elements.
In particular, `Allocator` must be either a stateless allocator, 
a <a href=https://en.cppreference.com/w/cpp/memory/polymorphic_allocator>std::pmr::polymorphic_allocator</a>, 
or a <a href=https://en.cppreference.com/w/cpp/memory/scoped_allocator_adaptor>std::scoped_allocator_adaptor</a>. 
Non-propagating stateful allocators, such as the [Boost.Interprocess allocators](https://www.boost.org/doc/libs/1_82_0/doc/html/interprocess/allocators_containers.html#interprocess.allocators_containers.allocator_introduction),
must be wrapped by a [std::scoped_allocator_adaptor](https://en.cppreference.com/w/cpp/memory/scoped_allocator_adaptor).

Every constructor has a version that accepts an allocator argument. 
A long string, byte string, array or object contains a pointer to underlying storage,
the allocator is used to allocate that storage, and it is retained in that storage.
For other data members the allocator argument is ignored.

#### Propagation

The allocator applies not only to a `basic_json`'s data member, but also to its data member's elements. For example:

```cpp
char buffer[1024];
std::pmr::monotonic_buffer_resource pool{ std::data(buffer), std::size(buffer) };
std::pmr::polymorphic_allocator<char> alloc(&pool);

std::string key1 = "KeyToLongForShortString";
std::string value1 = "string too long for short string";
std::string key2 = "AnotherKeyToLongForShortString";
std::string value2 = "another string too long for short string";

jsoncons::pmr::json j{ jsoncons::json_object_arg, alloc };
assert(j.get_allocator().resource() == &pool);
j[key1] = value1;
j.try_emplace(key2, value2);
auto it = std::search(std::begin(buffer), std::end(buffer), key1.begin(), key1.end());
assert(it != std::end(buffer));
it = std::search(std::begin(buffer), std::end(buffer), value1.begin(), value1.end());
assert(it != std::end(buffer));
it = std::search(std::begin(buffer), std::end(buffer), key2.begin(), key2.end());
assert(it != std::end(buffer));
it = std::search(std::begin(buffer), std::end(buffer), value2.begin(), value2.end());
assert(it != std::end(buffer));
```

#### Copy constructor

`basic_json` copy constructor 

```
Json j1(j);
```

constructs `j1` from `j`. If `j` holds a long string, bytes string, array or object,
copy construction applies allocator traits `select_on_container_copy_construction` to
the allocator from `j` (since 0.178.0) For example: 

```cpp
char buffer[1024];
std::pmr::monotonic_buffer_resource pool{ std::data(buffer), std::size(buffer) };
std::pmr::polymorphic_allocator<char> alloc(&pool);

jsoncons::pmr::json j{ "String too long for short string", alloc };
assert(j.is_string());
assert(j.get_allocator().resource() == &pool);

jsoncons::pmr::json j1(j);
assert(j1 == j);
assert(j1.get_allocator() == std::allocator_traits<std::pmr::polymorphic_allocator<char>>::
    select_on_container_copy_construction(j.get_allocator()));
assert(j1.get_allocator() == std::pmr::polymorphic_allocator<char>{}); // expected result for pmr allocators
```

#### Allocator-extended copy constructor

`basic_json` copy constructor 

```
Json j1(j, alloc);
```

constructs `j1` from `j`. If `j` holds a long string, bytes string, array or object,
copy construction uses allocator `alloc` for allocating storage, otherwise `alloc` is ignored. For example: 

```cpp
char buffer1[1024];
std::pmr::monotonic_buffer_resource pool1{ std::data(buffer1), std::size(buffer1) };
std::pmr::polymorphic_allocator<char> alloc1(&pool1);

char buffer2[1024];
std::pmr::monotonic_buffer_resource pool2{ std::data(buffer2), std::size(buffer2) };
std::pmr::polymorphic_allocator<char> alloc2(&pool2);

jsoncons::pmr::json j{ "String too long for short string", alloc1 };
assert(j.is_string());
assert(j.get_allocator().resource() == &pool1);

jsoncons::pmr::json j1(j, alloc2);
assert(j1 == j);
assert(j1.get_allocator().resource() == &pool2);
```

#### Move constructor

`basic_json` move construction 

```
Json j1(std::move(j));
```

initializes `j1` with the contents of `j`, which is either a pointer or a trivially copyable value,
and changes the value in `j` to `null`. For example: 

```
char buffer[1024];
std::pmr::monotonic_buffer_resource pool{ std::data(buffer), std::size(buffer) };
std::pmr::polymorphic_allocator<char> alloc(&pool);

jsoncons::pmr::json j{ "String too long for short string", alloc};
assert(j.is_string());
assert(j.get_allocator().resource() == &pool);

jsoncons::pmr::json j1(std::move(j));
assert(j1.is_string());
assert(j1.get_allocator().resource() == &pool);
assert(j.is_null());
```

#### Move assignment

If either `j` or `j1` are a long string, byte string, array, or object, `basic_json` move assignment 

```
j1 = std::move(j);
```

swaps the two data member values, such that two pointers are swapped or a pointer and a
trivially copyable value are swapped. Otherwise, move assignment copies `j`'s value to `j1`
and leaves `j` unchanged. For example:

```cpp
char buffer[1024];
std::pmr::monotonic_buffer_resource pool{ std::data(buffer), std::size(buffer) };
std::pmr::polymorphic_allocator<char> alloc(&pool);

jsoncons::pmr::json j{ "String too long for short string", alloc };
assert(j.is_string());
assert(j.get_allocator().resource() == &pool);

jsoncons::pmr::json j1{10};
assert(j1.is_number());

j1 = std::move(j);
assert(j1.is_string());
assert(j1.get_allocator().resource() == &pool);
assert(j.is_number());
```

#### Resources

[An allocator-aware variant type](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3153r0.html)

[C++ named requirements: Allocator](https://en.cppreference.com/w/cpp/named_req/Allocator)

[std::allocator_traits](https://en.cppreference.com/w/cpp/memory/allocator_traits)

[std::pmr::polymorphic_allocator](https://en.cppreference.com/w/cpp/memory/polymorphic_allocator)

[std::scoped_allocator_adaptor](https://en.cppreference.com/w/cpp/memory/scoped_allocator_adaptor)


