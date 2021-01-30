### jsoncons::jsonpath::result_flags

```c++
enum class result_flags 
{
   value = 1,
   path = 2,
   no_duplicates = 4 | path
};
```

A [BitmaskType](https://en.cppreference.com/w/cpp/named_req/BitmaskType) 
used to specify result options for `json_query` 
