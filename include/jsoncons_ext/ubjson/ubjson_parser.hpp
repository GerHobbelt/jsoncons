// Copyright 2017 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_UBJSON_UBJSON_PARSER_HPP
#define JSONCONS_UBJSON_UBJSON_PARSER_HPP

#include <string>
#include <memory>
#include <utility> // std::move
#include <jsoncons/json.hpp>
#include <jsoncons/source.hpp>
#include <jsoncons/json_visitor.hpp>
#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons_ext/ubjson/ubjson_detail.hpp>
#include <jsoncons_ext/ubjson/ubjson_error.hpp>
#include <jsoncons_ext/ubjson/ubjson_options.hpp>

namespace jsoncons { namespace ubjson {

enum class parse_mode {root,before_done,array,indefinite_array,strongly_typed_array,map_key,map_value,strongly_typed_map_key,strongly_typed_map_value,indefinite_map_key,indefinite_map_value};

struct parse_state 
{
    parse_mode mode; 
    std::size_t length;
    uint8_t type;
    std::size_t index;

    parse_state(parse_mode mode, std::size_t length, uint8_t type = 0)
        : mode(mode), length(length), type(type), index(0)
    {
    }

    parse_state(const parse_state&) = default;
    parse_state(parse_state&&) = default;
};

template <class Src,class Allocator=std::allocator<char>>
class basic_ubjson_parser : public ser_context
{
    using char_type = char;
    using char_traits_type = std::char_traits<char>;
    using temp_allocator_type = Allocator;
    using char_allocator_type = typename std::allocator_traits<temp_allocator_type>:: template rebind_alloc<char_type>;                  
    using byte_allocator_type = typename std::allocator_traits<temp_allocator_type>:: template rebind_alloc<uint8_t>;                  
    using parse_state_allocator_type = typename std::allocator_traits<temp_allocator_type>:: template rebind_alloc<parse_state>;                         

    Src source_;
    ubjson_decode_options options_;
    bool more_;
    bool done_;
    std::basic_string<char,std::char_traits<char>,char_allocator_type> text_buffer_;
    std::vector<parse_state,parse_state_allocator_type> state_stack_;
    int nesting_depth_;
public:
    template <class Source>
        basic_ubjson_parser(Source&& source,
                          const ubjson_decode_options& options = ubjson_decode_options(),
                          const Allocator alloc = Allocator())
       : source_(std::forward<Source>(source)), 
         options_(options),
         more_(true), 
         done_(false),
         text_buffer_(alloc),
         state_stack_(alloc),
         nesting_depth_(0)
    {
        state_stack_.emplace_back(parse_mode::root,0);
    }

    void restart()
    {
        more_ = true;
    }

    void reset()
    {
        state_stack_.clear();
        state_stack_.emplace_back(parse_mode::root,0);
        more_ = true;
        done_ = false;
    }

    bool done() const
    {
        return done_;
    }

    bool stopped() const
    {
        return !more_;
    }

    std::size_t line() const override
    {
        return 0;
    }

    std::size_t column() const override
    {
        return source_.position();
    }

    void parse(json_visitor& visitor, std::error_code& ec)
    {
        while (!done_ && more_)
        {
            switch (state_stack_.back().mode)
            {
                case parse_mode::array:
                {
                    if (state_stack_.back().index < state_stack_.back().length)
                    {
                        ++state_stack_.back().index;
                        read_type_and_value(visitor, ec);
                        if (ec)
                        {
                            return;
                        }
                    }
                    else
                    {
                        end_array(visitor, ec);
                    }
                    break;
                }
                case parse_mode::strongly_typed_array:
                {
                    if (state_stack_.back().index < state_stack_.back().length)
                    {
                        ++state_stack_.back().index;
                        read_value(visitor, state_stack_.back().type, ec);
                        if (ec)
                        {
                            return;
                        }
                    }
                    else
                    {
                        end_array(visitor, ec);
                    }
                    break;
                }
                case parse_mode::indefinite_array:
                {
                    int c = source_.peek();
                    switch (c)
                    {
                        case Src::traits_type::eof():
                            ec = ubjson_errc::unexpected_eof;
                            more_ = false;
                            return;
                        case jsoncons::ubjson::detail::ubjson_format::end_array_marker:
                            source_.ignore(1);
                            end_array(visitor, ec);
                            if (ec)
                            {
                                return;
                            }
                            break;
                        default:
                            read_type_and_value(visitor, ec);
                            if (ec)
                            {
                                return;
                            }
                            break;
                    }
                    break;
                }
                case parse_mode::map_key:
                {
                    if (state_stack_.back().index < state_stack_.back().length)
                    {
                        ++state_stack_.back().index;
                        read_name(visitor, ec);
                        if (ec)
                        {
                            return;
                        }
                        state_stack_.back().mode = parse_mode::map_value;
                    }
                    else
                    {
                        end_object(visitor, ec);
                    }
                    break;
                }
                case parse_mode::map_value:
                {
                    state_stack_.back().mode = parse_mode::map_key;
                    read_type_and_value(visitor, ec);
                    if (ec)
                    {
                        return;
                    }
                    break;
                }
                case parse_mode::strongly_typed_map_key:
                {
                    if (state_stack_.back().index < state_stack_.back().length)
                    {
                        ++state_stack_.back().index;
                        read_name(visitor, ec);
                        if (ec)
                        {
                            return;
                        }
                        state_stack_.back().mode = parse_mode::strongly_typed_map_value;
                    }
                    else
                    {
                        end_object(visitor, ec);
                    }
                    break;
                }
                case parse_mode::strongly_typed_map_value:
                {
                    state_stack_.back().mode = parse_mode::strongly_typed_map_key;
                    read_value(visitor, state_stack_.back().type, ec);
                    if (ec)
                    {
                        return;
                    }
                    break;
                }
                case parse_mode::indefinite_map_key:
                {
                    int c = source_.peek();
                    switch (c)
                    {
                        case Src::traits_type::eof():
                            ec = ubjson_errc::unexpected_eof;
                            more_ = false;
                            return;
                        case jsoncons::ubjson::detail::ubjson_format::end_array_marker:
                            source_.ignore(1);
                            end_object(visitor, ec);
                            if (ec)
                            {
                                return;
                            }
                            break;
                        default:
                            read_name(visitor, ec);
                            if (ec)
                            {
                                return;
                            }
                            state_stack_.back().mode = parse_mode::indefinite_map_value;
                            break;
                    }
                    break;
                }
                case parse_mode::indefinite_map_value:
                {
                    state_stack_.back().mode = parse_mode::indefinite_map_key;
                    read_type_and_value(visitor, ec);
                    if (ec)
                    {
                        return;
                    }
                    break;
                }
                case parse_mode::root:
                {
                    state_stack_.back().mode = parse_mode::before_done;
                    read_type_and_value(visitor, ec);
                    if (ec)
                    {
                        return;
                    }
                    break;
                }
                case parse_mode::before_done:
                {
                    JSONCONS_ASSERT(state_stack_.size() == 1);
                    state_stack_.clear();
                    more_ = false;
                    done_ = true;
                    visitor.flush();
                    break;
                }
            }
        }
    }
private:
    void read_type_and_value(json_visitor& visitor, std::error_code& ec)
    {
        if (source_.is_error())
        {
            ec = ubjson_errc::source_error;
            return;
        }   

        uint8_t type{};
        if (source_.get(type) == 0)
        {
            ec = ubjson_errc::unexpected_eof;
            return;
        }
        read_value(visitor, type, ec);
    }

    void read_value(json_visitor& visitor, uint8_t type, std::error_code& ec)
    {
        switch (type)
        {
            case jsoncons::ubjson::detail::ubjson_format::null_type: 
            {
                more_ = visitor.null_value(semantic_tag::none, *this, ec);
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::no_op_type: 
            {
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::true_type:
            {
                more_ = visitor.bool_value(true, semantic_tag::none, *this, ec);
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::false_type:
            {
                more_ = visitor.bool_value(false, semantic_tag::none, *this, ec);
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::int8_type: 
            {
                uint8_t buf[sizeof(int8_t)];
                source_.read(buf, sizeof(int8_t));
                if (source_.eof())
                {
                    ec = ubjson_errc::unexpected_eof;
                    return;
                }
                const uint8_t* endp;
                int8_t val = jsoncons::detail::big_to_native<int8_t>(buf,buf+sizeof(buf),&endp);
                more_ = visitor.int64_value(val, semantic_tag::none, *this, ec);
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::uint8_type: 
            {
                uint8_t val{};
                if (source_.get(val) == 0)
                {
                    ec = ubjson_errc::unexpected_eof;
                    return;
                }
                more_ = visitor.uint64_value(val, semantic_tag::none, *this, ec);
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::int16_type: 
            {
                uint8_t buf[sizeof(int16_t)];
                source_.read(buf, sizeof(int16_t));
                if (source_.eof())
                {
                    ec = ubjson_errc::unexpected_eof;
                    return;
                }
                const uint8_t* endp;
                int16_t val = jsoncons::detail::big_to_native<int16_t>(buf,buf+sizeof(buf),&endp);
                more_ = visitor.int64_value(val, semantic_tag::none, *this, ec);
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::int32_type: 
            {
                uint8_t buf[sizeof(int32_t)];
                source_.read(buf, sizeof(int32_t));
                if (source_.eof())
                {
                    ec = ubjson_errc::unexpected_eof;
                    return;
                }
                const uint8_t* endp;
                int32_t val = jsoncons::detail::big_to_native<int32_t>(buf,buf+sizeof(buf),&endp);
                more_ = visitor.int64_value(val, semantic_tag::none, *this, ec);
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::int64_type: 
            {
                uint8_t buf[sizeof(int64_t)];
                source_.read(buf, sizeof(int64_t));
                if (source_.eof())
                {
                    ec = ubjson_errc::unexpected_eof;
                    return;
                }
                const uint8_t* endp;
                int64_t val = jsoncons::detail::big_to_native<int64_t>(buf,buf+sizeof(buf),&endp);
                more_ = visitor.int64_value(val, semantic_tag::none, *this, ec);
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::float32_type: 
            {
                uint8_t buf[sizeof(float)];
                source_.read(buf, sizeof(float));
                if (source_.eof())
                {
                    ec = ubjson_errc::unexpected_eof;
                    return;
                }
                const uint8_t* endp;
                float val = jsoncons::detail::big_to_native<float>(buf,buf+sizeof(buf),&endp);
                more_ = visitor.double_value(val, semantic_tag::none, *this, ec);
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::float64_type: 
            {
                uint8_t buf[sizeof(double)];
                source_.read(buf, sizeof(double));
                if (source_.eof())
                {
                    ec = ubjson_errc::unexpected_eof;
                    return;
                }
                const uint8_t* endp;
                double val = jsoncons::detail::big_to_native<double>(buf,buf+sizeof(buf),&endp);
                more_ = visitor.double_value(val, semantic_tag::none, *this, ec);
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::char_type: 
            {
                uint8_t buf[sizeof(char)];
                source_.read(buf, sizeof(char));
                if (source_.eof())
                {
                    ec = ubjson_errc::unexpected_eof;
                    return;
                }
                const uint8_t* endp;
                char c = jsoncons::detail::big_to_native<char>(buf,buf+sizeof(buf),&endp);
                auto result = unicons::validate(&c,&c+1);
                if (result.ec != unicons::conv_errc())
                {
                    ec = ubjson_errc::invalid_utf8_text_string;
                    return;
                }
                more_ = visitor.string_value(basic_string_view<char>(&c,1), semantic_tag::none, *this, ec);
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::string_type: 
            {
                std::size_t length = get_length(ec);
                if (ec)
                {
                    return;
                }
                text_buffer_.clear();
                source_.read(std::back_inserter(text_buffer_), length);
                if (source_.eof())
                {
                    ec = ubjson_errc::unexpected_eof;
                    return;
                }
                auto result = unicons::validate(text_buffer_.begin(),text_buffer_.end());
                if (result.ec != unicons::conv_errc())
                {
                    ec = ubjson_errc::invalid_utf8_text_string;
                    return;
                }
                more_ = visitor.string_value(basic_string_view<char>(text_buffer_.data(),text_buffer_.length()), semantic_tag::none, *this, ec);
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::high_precision_number_type: 
            {
                std::size_t length = get_length(ec);
                if (ec)
                {
                    return;
                }
                text_buffer_.clear();
                source_.read(std::back_inserter(text_buffer_), length);
                if (source_.eof())
                {
                    ec = ubjson_errc::unexpected_eof;
                    return;
                }
                if (jsoncons::detail::is_base10(text_buffer_.data(),text_buffer_.length()))
                {
                    more_ = visitor.string_value(basic_string_view<char>(text_buffer_.data(),text_buffer_.length()), semantic_tag::bigint, *this);
                }
                else
                {
                    more_ = visitor.string_value(basic_string_view<char>(text_buffer_.data(),text_buffer_.length()), semantic_tag::bigdec, *this);
                }
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::start_array_marker: 
            {
                begin_array(visitor,ec);
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::start_object_marker: 
            {
                begin_object(visitor, ec);
                break;
            }
            default:
            {
                ec = ubjson_errc::unknown_type;
                return;
            }
        }
    }

    void begin_array(json_visitor& visitor, std::error_code& ec)
    {
        if (JSONCONS_UNLIKELY(++nesting_depth_ > options_.max_depth()))
        {
            ec = json_errc::max_depth_exceeded;
            return;
        } 
        if (source_.peek() == jsoncons::ubjson::detail::ubjson_format::type_marker)
        {
            source_.ignore(1);
            uint8_t item_type{};
            if (source_.get(item_type) == 0)
            {
                ec = ubjson_errc::unexpected_eof;
                return;
            }
            if (source_.peek() == jsoncons::ubjson::detail::ubjson_format::count_marker)
            {
                source_.ignore(1);
                std::size_t length = get_length(ec);
                state_stack_.emplace_back(parse_mode::strongly_typed_array,length,item_type);
                more_ = visitor.begin_array(length, semantic_tag::none, *this, ec);
            }
            else
            {
                ec = ubjson_errc::count_required_after_type;
                return;
            }
        }
        else if (source_.peek() == jsoncons::ubjson::detail::ubjson_format::count_marker)
        {
            source_.ignore(1);
            std::size_t length = get_length(ec);
            state_stack_.emplace_back(parse_mode::array,length);
            more_ = visitor.begin_array(length, semantic_tag::none, *this, ec);
        }
        else
        {
            state_stack_.emplace_back(parse_mode::indefinite_array,0);
            more_ = visitor.begin_array(semantic_tag::none, *this, ec);
        }
    }

    void end_array(json_visitor& visitor, std::error_code&)
    {
        --nesting_depth_;

        more_ = visitor.end_array(*this);
        state_stack_.pop_back();
    }

    void begin_object(json_visitor& visitor, std::error_code& ec)
    {
        if (JSONCONS_UNLIKELY(++nesting_depth_ > options_.max_depth()))
        {
            ec = json_errc::max_depth_exceeded;
            return;
        } 
        if (source_.peek() == jsoncons::ubjson::detail::ubjson_format::type_marker)
        {
            source_.ignore(1);
            uint8_t item_type{};
            if (source_.get(item_type) == 0)
            {
                ec = ubjson_errc::unexpected_eof;
                return;
            }
            if (source_.peek() == jsoncons::ubjson::detail::ubjson_format::count_marker)
            {
                source_.ignore(1);
                std::size_t length = get_length(ec);
                state_stack_.emplace_back(parse_mode::strongly_typed_map_key,length,item_type);
                more_ = visitor.begin_object(length, semantic_tag::none, *this, ec);
            }
            else
            {
                ec = ubjson_errc::count_required_after_type;
                return;
            }
        }
        else
        {
            if (source_.peek() == jsoncons::ubjson::detail::ubjson_format::count_marker)
            {
                source_.ignore(1);
                std::size_t length = get_length(ec);
                state_stack_.emplace_back(parse_mode::map_key,length);
                more_ = visitor.begin_object(length, semantic_tag::none, *this, ec);
            }
            else
            {
                state_stack_.emplace_back(parse_mode::indefinite_map_key,0);
                more_ = visitor.begin_object(semantic_tag::none, *this, ec);
            }
        }
    }

    void end_object(json_visitor& visitor, std::error_code&)
    {
        --nesting_depth_;
        more_ = visitor.end_object(*this);
        state_stack_.pop_back();
    }

    std::size_t get_length(std::error_code& ec)
    {
        std::size_t length = 0;
        if (JSONCONS_UNLIKELY(source_.eof()))
        {
            ec = ubjson_errc::unexpected_eof;
            return length;
        }
        uint8_t type{};
        if (source_.get(type) == 0)
        {
            ec = ubjson_errc::unexpected_eof;
            return length;
        }
        switch (type)
        {
            case jsoncons::ubjson::detail::ubjson_format::int8_type: 
            {
                uint8_t buf[sizeof(int8_t)];
                source_.read(buf, sizeof(int8_t));
                if (source_.eof())
                {
                    ec = ubjson_errc::unexpected_eof;
                    return length;
                }
                const uint8_t* endp;
                int8_t val = jsoncons::detail::big_to_native<int8_t>(buf,buf+sizeof(buf),&endp);
                if (val >= 0)
                {
                    length = val;
                }
                else
                {
                    ec = ubjson_errc::length_cannot_be_negative;
                    return length;
                }
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::uint8_type: 
            {
                uint8_t val{};
                if (source_.get(val) == 0)
                {
                    ec = ubjson_errc::unexpected_eof;
                    return length;
                }
                length = val;
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::int16_type: 
            {
                uint8_t buf[sizeof(int16_t)];
                source_.read(buf, sizeof(int16_t));
                if (source_.eof())
                {
                    ec = ubjson_errc::unexpected_eof;
                    return length;
                }
                const uint8_t* endp;
                int16_t val = jsoncons::detail::big_to_native<int16_t>(buf,buf+sizeof(buf),&endp);
                if (val >= 0)
                {
                    length = val;
                }
                else
                {
                    ec = ubjson_errc::length_cannot_be_negative;
                    return length;
                }
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::int32_type: 
            {
                uint8_t buf[sizeof(int32_t)];
                source_.read(buf, sizeof(int32_t));
                if (source_.eof())
                {
                    ec = ubjson_errc::unexpected_eof;
                    return length;
                }
                const uint8_t* endp;
                int32_t val = jsoncons::detail::big_to_native<int32_t>(buf,buf+sizeof(buf),&endp);
                if (val >= 0)
                {
                    length = val;
                }
                else
                {
                    ec = ubjson_errc::length_cannot_be_negative;
                    return length;
                }
                break;
            }
            case jsoncons::ubjson::detail::ubjson_format::int64_type: 
            {
                uint8_t buf[sizeof(int64_t)];
                source_.read(buf, sizeof(int64_t));
                if (source_.eof())
                {
                    ec = ubjson_errc::unexpected_eof;
                    return length;
                }
                const uint8_t* endp;
                int64_t val = jsoncons::detail::big_to_native<int64_t>(buf,buf+sizeof(buf),&endp);
                if (val >= 0)
                {
                    length = (std::size_t)val;
                    if (length != (uint64_t)val)
                    {
                        ec = ubjson_errc::number_too_large;
                        return length;
                    }
                }
                else
                {
                    ec = ubjson_errc::length_cannot_be_negative;
                    return length;
                }
                break;
            }
            default:
            {
                ec = ubjson_errc::length_must_be_integer;
                return length;
            }
        }
        return length;
    }

    void read_name(json_visitor& visitor, std::error_code& ec)
    {
        std::size_t length = get_length(ec);
        if (ec)
        {
            return;
        }
        text_buffer_.clear();
        source_.read(std::back_inserter(text_buffer_), length);
        if (source_.eof())
        {
            ec = ubjson_errc::unexpected_eof;
            return;
        }
        auto result = unicons::validate(text_buffer_.begin(),text_buffer_.end());
        if (result.ec != unicons::conv_errc())
        {
            ec = ubjson_errc::invalid_utf8_text_string;
            return;
        }
        more_ = visitor.key(basic_string_view<char>(text_buffer_.data(),text_buffer_.length()), *this);
    }
};

}}

#endif
