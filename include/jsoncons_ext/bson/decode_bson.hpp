// Copyright 2013-2023 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_BSON_DECODE_BSON_HPP
#define JSONCONS_BSON_DECODE_BSON_HPP

#include <string>
#include <vector>
#include <memory>
#include <type_traits> // std::enable_if
#include <istream> // std::basic_istream
#include <jsoncons/json.hpp>
#include <jsoncons/wrapped_allocators.hpp>
#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons_ext/bson/bson_reader.hpp>
#include <jsoncons_ext/bson/bson_cursor.hpp>

namespace jsoncons { 
namespace bson {

    template<class T, class Source>
    typename std::enable_if<extension_traits::is_basic_json<T>::value &&
                            extension_traits::is_byte_sequence<Source>::value,T>::type 
    decode_bson(const Source& v, 
                const bson_decode_options& options = bson_decode_options())
    {
        jsoncons::json_decoder<T> decoder;
        auto adaptor = make_json_visitor_adaptor<json_visitor>(decoder);
        basic_bson_reader<jsoncons::bytes_source> reader(v, adaptor, options);
        reader.read();
        if (!decoder.is_valid())
        {
            JSONCONS_THROW(ser_error(conv_errc::conversion_failed, reader.line(), reader.column()));
        }
        return decoder.get_result();
    }

    template<class T, class Source>
    typename std::enable_if<!extension_traits::is_basic_json<T>::value &&
                            extension_traits::is_byte_sequence<Source>::value,T>::type 
    decode_bson(const Source& v, 
                const bson_decode_options& options = bson_decode_options())
    {
        basic_bson_cursor<bytes_source> cursor(v, options);
        json_decoder<basic_json<char,sorted_policy>> decoder{};

        std::error_code ec;
        T val = decode_traits<T,char>::decode(cursor, decoder, ec);
        if (ec)
        {
            JSONCONS_THROW(ser_error(ec, cursor.context().line(), cursor.context().column()));
        }
        return val;
    }

    template<class T>
    typename std::enable_if<extension_traits::is_basic_json<T>::value,T>::type 
    decode_bson(std::istream& is, 
                const bson_decode_options& options = bson_decode_options())
    {
        jsoncons::json_decoder<T> decoder;
        auto adaptor = make_json_visitor_adaptor<json_visitor>(decoder);
        bson_stream_reader reader(is, adaptor, options);
        reader.read();
        if (!decoder.is_valid())
        {
            JSONCONS_THROW(ser_error(conv_errc::conversion_failed, reader.line(), reader.column()));
        }
        return decoder.get_result();
    }

    template<class T>
    typename std::enable_if<!extension_traits::is_basic_json<T>::value,T>::type 
    decode_bson(std::istream& is, 
                const bson_decode_options& options = bson_decode_options())
    {
        basic_bson_cursor<binary_stream_source> cursor(is, options);
        json_decoder<basic_json<char,sorted_policy>> decoder{};

        std::error_code ec;
        T val = decode_traits<T,char>::decode(cursor, decoder, ec);
        if (ec)
        {
            JSONCONS_THROW(ser_error(ec, cursor.context().line(), cursor.context().column()));
        }
        return val;
    }

    template<class T, class InputIt>
    typename std::enable_if<extension_traits::is_basic_json<T>::value,T>::type 
    decode_bson(InputIt first, InputIt last,
                const bson_decode_options& options = bson_decode_options())
    {
        jsoncons::json_decoder<T> decoder;
        auto adaptor = make_json_visitor_adaptor<json_visitor>(decoder);
        basic_bson_reader<binary_iterator_source<InputIt>> reader(binary_iterator_source<InputIt>(first, last), adaptor, options);
        reader.read();
        if (!decoder.is_valid())
        {
            JSONCONS_THROW(ser_error(conv_errc::conversion_failed, reader.line(), reader.column()));
        }
        return decoder.get_result();
    }

    template<class T, class InputIt>
    typename std::enable_if<!extension_traits::is_basic_json<T>::value,T>::type 
    decode_bson(InputIt first, InputIt last,
                const bson_decode_options& options = bson_decode_options())
    {
        basic_bson_cursor<binary_iterator_source<InputIt>> cursor(binary_iterator_source<InputIt>(first, last), options);
        json_decoder<basic_json<char,sorted_policy>> decoder{};

        std::error_code ec;
        T val = decode_traits<T,char>::decode(cursor, decoder, ec);
        if (ec)
        {
            JSONCONS_THROW(ser_error(ec, cursor.context().line(), cursor.context().column()));
        }
        return val;
    }

    // With leading wrapped_allocators parameter

    template<class T,class Source,class ResultAllocator,class WorkAllocator>
    typename std::enable_if<extension_traits::is_basic_json<T>::value &&
                            extension_traits::is_byte_sequence<Source>::value,T>::type 
    decode_bson(const wrapped_allocators<ResultAllocator,WorkAllocator>& allocators,
                const Source& v, 
                const bson_decode_options& options = bson_decode_options())
    {
        json_decoder<T,WorkAllocator> decoder(allocators.get_work_allocator());
        auto adaptor = make_json_visitor_adaptor<json_visitor>(decoder);
        basic_bson_reader<jsoncons::bytes_source,WorkAllocator> reader(v, adaptor, options, allocators.get_work_allocator());
        reader.read();
        if (!decoder.is_valid())
        {
            JSONCONS_THROW(ser_error(conv_errc::conversion_failed, reader.line(), reader.column()));
        }
        return decoder.get_result();
    }

    template<class T, class Source,class ResultAllocator,class WorkAllocator>
    typename std::enable_if<!extension_traits::is_basic_json<T>::value &&
                            extension_traits::is_byte_sequence<Source>::value,T>::type 
    decode_bson(const wrapped_allocators<ResultAllocator,WorkAllocator>& allocators,
                const Source& v, 
                const bson_decode_options& options = bson_decode_options())
    {
        basic_bson_cursor<bytes_source,WorkAllocator> cursor(v, options, allocators.get_work_allocator());
        json_decoder<basic_json<char,sorted_policy,WorkAllocator>,WorkAllocator> decoder(allocators.get_work_allocator(), allocators.get_work_allocator());

        std::error_code ec;
        T val = decode_traits<T,char>::decode(cursor, decoder, ec);
        if (ec)
        {
            JSONCONS_THROW(ser_error(ec, cursor.context().line(), cursor.context().column()));
        }
        return val;
    }

    template<class T,class ResultAllocator,class WorkAllocator>
    typename std::enable_if<extension_traits::is_basic_json<T>::value,T>::type 
    decode_bson(const wrapped_allocators<ResultAllocator,WorkAllocator>& allocators,
                std::istream& is, 
                const bson_decode_options& options = bson_decode_options())
    {
        json_decoder<T,WorkAllocator> decoder(allocators.get_work_allocator());
        auto adaptor = make_json_visitor_adaptor<json_visitor>(decoder);
        basic_bson_reader<jsoncons::binary_stream_source,WorkAllocator> reader(is, adaptor, options, allocators.get_work_allocator());
        reader.read();
        if (!decoder.is_valid())
        {
            JSONCONS_THROW(ser_error(conv_errc::conversion_failed, reader.line(), reader.column()));
        }
        return decoder.get_result();
    }

    template<class T,class ResultAllocator,class WorkAllocator>
    typename std::enable_if<!extension_traits::is_basic_json<T>::value,T>::type 
    decode_bson(const wrapped_allocators<ResultAllocator,WorkAllocator>& allocators,
                std::istream& is, 
                const bson_decode_options& options = bson_decode_options())
    {
        basic_bson_cursor<binary_stream_source,WorkAllocator> cursor(is, options, allocators.get_work_allocator());
        json_decoder<basic_json<char,sorted_policy,WorkAllocator>,WorkAllocator> decoder(allocators.get_work_allocator(), allocators.get_work_allocator());

        std::error_code ec;
        T val = decode_traits<T,char>::decode(cursor, decoder, ec);
        if (ec)
        {
            JSONCONS_THROW(ser_error(ec, cursor.context().line(), cursor.context().column()));
        }
        return val;
    }
  
} // bson
} // jsoncons

#endif
