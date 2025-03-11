// Copyright 2013-2024 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSON_READER_HPP
#define JSONCONS_JSON_READER_HPP

#include <memory> // std::allocator
#include <string>
#include <vector>
#include <stdexcept>
#include <system_error>
#include <ios>
#include <utility> // std::move
#include <jsoncons/source.hpp>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/json_visitor.hpp>
#include <jsoncons/json_parser.hpp>
#include <jsoncons/source_adaptor.hpp>

namespace jsoncons {

    // utf8_other_json_input_adapter

    template <typename CharT>
    class json_utf8_to_other_visitor_adaptor : public json_visitor
    {
    public:
        using json_visitor::string_view_type;
    private:
        basic_default_json_visitor<CharT> default_visitor_;
        basic_json_visitor<CharT>& other_visitor_;

        // noncopyable and nonmoveable
        json_utf8_to_other_visitor_adaptor(const json_utf8_to_other_visitor_adaptor<CharT>&) = delete;
        json_utf8_to_other_visitor_adaptor<CharT>& operator=(const json_utf8_to_other_visitor_adaptor<CharT>&) = delete;

    public:
        json_utf8_to_other_visitor_adaptor()
            : other_visitor_(default_visitor_)
        {
        }

        json_utf8_to_other_visitor_adaptor(basic_json_visitor<CharT>& other_visitor)
            : other_visitor_(other_visitor)
        {
        }

    private:

        void visit_flush() override
        {
            other_visitor_.flush();
        }

        bool visit_begin_object(semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            return other_visitor_.begin_object(tag, context, ec);
        }

        bool visit_end_object(const ser_context& context, std::error_code& ec) override
        {
            return other_visitor_.end_object(context, ec);
        }

        bool visit_begin_array(semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            return other_visitor_.begin_array(tag, context, ec);
        }

        bool visit_end_array(const ser_context& context, std::error_code& ec) override
        {
            return other_visitor_.end_array(context, ec);
        }

        bool visit_key(const string_view_type& name, const ser_context& context, std::error_code& ec) override
        {
            std::basic_string<CharT> target;
            auto result = unicode_traits::convert(
                name.data(), name.size(), target, 
                unicode_traits::conv_flags::strict);
            if (result.ec != unicode_traits::conv_errc())
            {
                JSONCONS_THROW(ser_error(result.ec,context.line(),context.column()));
            }
            return other_visitor_.key(target, context, ec);
        }

        bool visit_string(const string_view_type& value, semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            std::basic_string<CharT> target;
            auto result = unicode_traits::convert(
                value.data(), value.size(), target, 
                unicode_traits::conv_flags::strict);
            if (result.ec != unicode_traits::conv_errc())
            {
                ec = result.ec;
                return false;
            }
            return other_visitor_.string_value(target, tag, context, ec);
        }

        bool visit_int64(int64_t value, 
                            semantic_tag tag, 
                            const ser_context& context,
                            std::error_code& ec) override
        {
            return other_visitor_.int64_value(value, tag, context, ec);
        }

        bool visit_uint64(uint64_t value, 
                             semantic_tag tag, 
                             const ser_context& context,
                             std::error_code& ec) override
        {
            return other_visitor_.uint64_value(value, tag, context, ec);
        }

        bool visit_half(uint16_t value, 
                           semantic_tag tag,
                           const ser_context& context,
                           std::error_code& ec) override
        {
            return other_visitor_.half_value(value, tag, context, ec);
        }

        bool visit_double(double value, 
                             semantic_tag tag,
                             const ser_context& context,
                             std::error_code& ec) override
        {
            return other_visitor_.double_value(value, tag, context, ec);
        }

        bool visit_bool(bool value, semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            return other_visitor_.bool_value(value, tag, context, ec);
        }

        bool visit_null(semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            return other_visitor_.null_value(tag, context, ec);
        }
    };

    template <typename CharT,typename Source=jsoncons::stream_source<CharT>,typename TempAllocator =std::allocator<char>>
    class basic_json_reader final : private chunk_reader<CharT> 
    {
    public:
        using char_type = CharT;
        using source_type = Source;
        using string_view_type = jsoncons::basic_string_view<CharT>;
    private:
        using char_allocator_type = typename std::allocator_traits<TempAllocator>:: template rebind_alloc<CharT>;

        static constexpr size_t default_max_buffer_size = 16384;

        json_source_adaptor<Source> source_;
        basic_default_json_visitor<CharT> default_visitor_;
        basic_json_visitor<CharT>& visitor_;
        basic_json_parser<CharT,TempAllocator> parser_;
        bool eof_;

        // Noncopyable and nonmoveable
        basic_json_reader(const basic_json_reader&) = delete;
        basic_json_reader& operator=(const basic_json_reader&) = delete;

    public:
        template <typename Sourceable>
        explicit basic_json_reader(Sourceable&& source, const TempAllocator& temp_alloc = TempAllocator())
            : basic_json_reader(std::forward<Sourceable>(source),
                                default_visitor_,
                                basic_json_decode_options<CharT>(),
                                temp_alloc)
        {
        }

        template <typename Sourceable>
        basic_json_reader(Sourceable&& source, 
                          const basic_json_decode_options<CharT>& options, 
                          const TempAllocator& temp_alloc = TempAllocator())
            : basic_json_reader(std::forward<Sourceable>(source),
                                default_visitor_,
                                options,
                                temp_alloc)
        {
        }

        template <typename Sourceable>
        basic_json_reader(Sourceable&& source, 
                          basic_json_visitor<CharT>& visitor, 
                          const TempAllocator& temp_alloc = TempAllocator())
            : basic_json_reader(std::forward<Sourceable>(source),
                                visitor,
                                basic_json_decode_options<CharT>(),
                                temp_alloc)
        {
        }

        template <typename Sourceable>
        basic_json_reader(Sourceable&& source, 
                          basic_json_visitor<CharT>& visitor,
                          const basic_json_decode_options<CharT>& options, 
                          const TempAllocator& temp_alloc = TempAllocator())
            : source_(std::forward<Sourceable>(source)),
              visitor_(visitor),
              parser_(this, options, options.err_handler(), temp_alloc),
              eof_(false)
        {
        }

#if !defined(JSONCONS_NO_DEPRECATED)
        template <typename Sourceable>
        JSONCONS_DEPRECATED_MSG("Instead, set err_handler in options")
        basic_json_reader(Sourceable&& source,
                          std::function<bool(json_errc,const ser_context&)> err_handler, 
                          const TempAllocator& temp_alloc = TempAllocator())
            : basic_json_reader(std::forward<Sourceable>(source),
                                default_visitor_,
                                basic_json_decode_options<CharT>(),
                                err_handler,
                                temp_alloc)
        {
        }

        template <typename Sourceable>
        JSONCONS_DEPRECATED_MSG("Instead, set err_handler in options")
        basic_json_reader(Sourceable&& source, 
                          const basic_json_decode_options<CharT>& options,
                          std::function<bool(json_errc,const ser_context&)> err_handler, 
                          const TempAllocator& temp_alloc = TempAllocator())
            : basic_json_reader(std::forward<Sourceable>(source),
                                default_visitor_,
                                options,
                                err_handler,
                                temp_alloc)
        {
        }

        template <typename Sourceable>
        JSONCONS_DEPRECATED_MSG("Instead, set err_handler in options")
        basic_json_reader(Sourceable&& source,
                          basic_json_visitor<CharT>& visitor,
                          std::function<bool(json_errc,const ser_context&)> err_handler, 
                          const TempAllocator& temp_alloc = TempAllocator())
            : basic_json_reader(std::forward<Sourceable>(source),
                                visitor,
                                basic_json_decode_options<CharT>(),
                                err_handler,
                                temp_alloc)
        {
        }

        template <typename Sourceable> 
        JSONCONS_DEPRECATED_MSG("Instead, set err_handler in options")
        basic_json_reader(Sourceable&& source,
                          basic_json_visitor<CharT>& visitor, 
                          const basic_json_decode_options<CharT>& options,
                          std::function<bool(json_errc,const ser_context&)> err_handler, 
                          const TempAllocator& temp_alloc = TempAllocator())
           : source_(std::forward<Sourceable>(source)),
             visitor_(visitor),
             parser_(this, options, err_handler, temp_alloc),
             eof_(false)
        {
        }
#endif

        void read_next()
        {
            std::error_code ec;
            read_next(ec);
            if (ec)
            {
                JSONCONS_THROW(ser_error(ec,parser_.line(),parser_.column()));
            }
        }

        void read_next(std::error_code& ec)
        {
            if (source_.is_error())
            {
                ec = json_errc::source_error;
                return;
            }        
            parser_.reset();
            auto s = source_.read_buffer(ec);
            if (ec) return;
            if (s.size() > 0)
            {
                parser_.set_buffer(s.data(),s.size());
            }
            parser_.parse_some(visitor_, ec);
            if (ec) return;
            if (!parser_.enter() && !parser_.accept())
            {
                ec = json_errc::unexpected_eof;
                return;
            }
            
            parser_.skip_space(ec);
        }

        void check_done()
        {
            std::error_code ec;
            check_done(ec);
            if (ec)
            {
                JSONCONS_THROW(ser_error(ec,parser_.line(),parser_.column()));
            }
        }

        std::size_t line() const
        {
            return parser_.line();
        }

        std::size_t column() const
        {
            return parser_.column();
        }

        void check_done(std::error_code& ec)
        {
            if (source_.is_error())
            {
                ec = json_errc::source_error;
                return;
            }   
            parser_.check_done(ec);
        }

        bool eof() const
        {
            return parser_.source_exhausted() && source_.eof();
        }

        void read()
        {
            read_next();
            check_done();
        }

        void read(std::error_code& ec)
        {
            read_next(ec);
            if (!ec)
            {
                check_done(ec);
            }
        }
    private:
        
        bool read_chunk(basic_parser_input<char_type>&, std::error_code& ec) final
        {
            //std::cout << "UPDATE BUFFER\n";
            bool success = false;
            auto s = source_.read_buffer(ec);
            if (ec) return false;
            if (s.size() > 0)
            {
                parser_.set_buffer(s.data(),s.size());
                success = true;
            }
            else
            {
                eof_ = true;
            }

            return success;
        }
    };

    using json_string_reader = basic_json_reader<char,string_source<char>>;
    using wjson_string_reader = basic_json_reader<wchar_t,string_source<wchar_t>>;
    using json_stream_reader = basic_json_reader<char,stream_source<char>>;
    using wjson_stream_reader = basic_json_reader<wchar_t,stream_source<wchar_t>>;

}

#endif

