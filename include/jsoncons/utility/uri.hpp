// Copyright 2013-2024 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_UTILITY_URI_HPP
#define JSONCONS_UTILITY_URI_HPP

#include <string> // std::string
#include <algorithm> 
#include <sstream> 
#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/detail/parse_number.hpp>
#include <jsoncons/detail/write_number.hpp>
#include <iostream>

namespace jsoncons { 

    enum class uri_errc
    {
        success = 0,
        invalid_uri = 1,
        invalid_characters_in_path = 2
    };


    class uri_error_category_impl
        : public std::error_category
    {
    public:
        const char* name() const noexcept override
        {
            return "jsoncons/uri";
        }
        std::string message(int ev) const override
        {
            switch (static_cast<uri_errc>(ev))
            {
                case uri_errc::invalid_uri:
                    return "Invalid URI";
                case uri_errc::invalid_characters_in_path:
                    return "Invalid characters in path";
                default:
                    return "Unknown uri error";
            }
        }
    };

    inline
        const std::error_category& uri_error_category()
    {
        static uri_error_category_impl instance;
        return instance;
    }

    inline
        std::error_code make_error_code(uri_errc result)
    {
        return std::error_code(static_cast<int>(result), uri_error_category());
    }

} // namespace jsoncons

namespace std {
    template<>
    struct is_error_code_enum<jsoncons::uri_errc> : public true_type
    {
    };
} // namespace std

namespace jsoncons {

    struct uri_fragment_part_t
    {
        explicit uri_fragment_part_t() = default; 
    };

    constexpr uri_fragment_part_t uri_fragment_part{};

    struct uri_raw_parts_t
    {
        explicit uri_raw_parts_t() = default; 
    };

    constexpr uri_raw_parts_t uri_raw_parts{};

    class uri
    {
        using part_type = std::pair<std::size_t,std::size_t>;

        std::string uri_string_;
        part_type scheme_;
        part_type userinfo_;
        part_type host_;
        part_type port_;
        part_type path_;
        part_type query_;
        part_type fragment_;
    public:

        uri()
            : uri_string_{}, scheme_{0,0},userinfo_{0,0},host_{0,0},port_{0,0},path_{0,0},query_{0,0},fragment_{0,0} 
        {
        }

        uri(const uri& other)
            : uri_string_(other.uri_string_), scheme_(other.scheme_), userinfo_(other.userinfo_), host_(other.host_), 
              port_(other.port_), path_(other.path_), query_(other.query_), fragment_(other.fragment_)
        {
        }

        uri(uri&& other) noexcept
            : uri_string_(std::move(other.uri_string_)), scheme_(other.scheme_), userinfo_(other.userinfo_), host_(other.host_), 
              port_(other.port_), path_(other.path_), query_(other.query_), fragment_(other.fragment_)
        {
        }

        uri(const uri& other, uri_fragment_part_t, jsoncons::string_view fragment)
            : uri_string_(other.uri_string_), scheme_(other.scheme_), userinfo_(other.userinfo_), host_(other.host_), 
              port_(other.port_), path_(other.path_), query_(other.query_)
        {
            uri_string_.erase(query_.second);
            if (!fragment.empty()) 
            {
                uri_string_.append("#");
                fragment_.first = uri_string_.length();
                encode_illegal_characters(fragment, uri_string_);
                fragment_.second = uri_string_.length();
            }
            else
            {
                fragment_.first = fragment_.second = uri_string_.length();
            }
        }

        explicit uri(const std::string& str)
        {
            std::error_code ec;
            *this = parse(str, ec);
            if (ec)
            {
                JSONCONS_THROW(std::system_error(ec));
            }
        }

        uri(jsoncons::string_view scheme,
            jsoncons::string_view userinfo,
            jsoncons::string_view host,
            jsoncons::string_view port,
            jsoncons::string_view path,
            jsoncons::string_view query = "",
            jsoncons::string_view fragment = "")
        {
            if (!scheme.empty()) 
            {
                uri_string_.append(scheme.data(), scheme.size());
                scheme_.second = uri_string_.length();
            }
            if (!userinfo.empty() || !host.empty() || !port.empty()) 
            {
                if (!scheme.empty()) 
                {
                    uri_string_.append("://");
                }

                if (!userinfo.empty()) 
                {
                    userinfo_.first = uri_string_.length();
                    encode_userinfo(userinfo, uri_string_);
                    userinfo_.second = uri_string_.length();
                    uri_string_.append("@");
                }
                else
                {
                    userinfo_.first = userinfo_.second = uri_string_.length();
                }

                if (!host.empty()) 
                {
                    host_.first = uri_string_.length();
                    uri_string_.append(host.data(), host.size());
                    host_.second = uri_string_.length();
                } 
                else 
                {
                    JSONCONS_THROW(json_runtime_error<std::invalid_argument>("uri error."));
                }

                if (!port.empty()) 
                {
                    uri_string_.append(":");
                    port_.first = uri_string_.length();
                    uri_string_.append(port.data(), port.size());
                    port_.second = uri_string_.length();
                }
                else
                {
                    port_.first = port_.second = uri_string_.length();
                }
            }
            else 
            {
                userinfo_.first = userinfo_.second = uri_string_.length();
                host_.first = host_.second = uri_string_.length();
                port_.first = port_.second = uri_string_.length();
                if (!scheme.empty())
                {
                    if (!path.empty() || !query.empty() || !fragment.empty()) 
                    {
                        uri_string_.append(":");
                    } 
                    else 
                    {
                        JSONCONS_THROW(json_runtime_error<std::invalid_argument>("uri error."));
                    }
                }
            }

            if (!path.empty()) 
            {
                // if the URI is not opaque and the path is not already prefixed
                // with a '/', add one.
                path_.first = uri_string_.length();
                if (!host.empty() && (path.front() != '/')) 
                {
                    uri_string_.push_back('/');
                }
                encode_path(path, uri_string_);
                path_.second = uri_string_.length();
            }
            else
            {
                path_.first = path_.second = uri_string_.length();
            }

            if (!query.empty()) 
            {
                uri_string_.append("?");
                query_.first = uri_string_.length();
                encode_illegal_characters(query, uri_string_);
                query_.second = uri_string_.length();
            }
            else
            {
                query_.first = query_.second = uri_string_.length();
            }

            if (!fragment.empty()) 
            {
                uri_string_.append("#");
                fragment_.first = uri_string_.length();
                encode_illegal_characters(fragment, uri_string_);
                fragment_.second = uri_string_.length();
            }
            else
            {
                fragment_.first = fragment_.second = uri_string_.length();
            }
        }

        uri(uri_raw_parts_t,
            jsoncons::string_view scheme,
            jsoncons::string_view userinfo,
            jsoncons::string_view host,
            jsoncons::string_view port,
            jsoncons::string_view path,
            jsoncons::string_view query,
            jsoncons::string_view fragment)
        {
            if (!scheme.empty()) 
            {
                uri_string_.append(scheme.data(), scheme.size());
                scheme_.second = uri_string_.length();
            }
            if (!userinfo.empty() || !host.empty() || !port.empty()) 
            {
                if (!scheme.empty()) 
                {
                    uri_string_.append("://");
                }

                if (!userinfo.empty()) 
                {
                    userinfo_.first = uri_string_.length();
                    uri_string_.append(userinfo.data(), userinfo.size());
                    userinfo_.second = uri_string_.length();
                    uri_string_.append("@");
                }
                else
                {
                    userinfo_.first = userinfo_.second = uri_string_.length();
                }

                if (!host.empty()) 
                {
                    host_.first = uri_string_.length();
                    uri_string_.append(host.data(), host.size());
                    host_.second = uri_string_.length();
                } 
                else 
                {
                    JSONCONS_THROW(json_runtime_error<std::invalid_argument>("uri error."));
                }

                if (!port.empty()) 
                {
                    uri_string_.append(":");
                    port_.first = uri_string_.length();
                    uri_string_.append(port.data(), port.size());
                    port_.second = uri_string_.length();
                }
                else
                {
                    port_.first = port_.second = uri_string_.length();
                }
            }
            else 
            {
                userinfo_.first = userinfo_.second = uri_string_.length();
                host_.first = host_.second = uri_string_.length();
                port_.first = port_.second = uri_string_.length();
                if (!scheme.empty())
                {
                    if (!path.empty() || !query.empty() || !fragment.empty()) 
                    {
                        uri_string_.append(":");
                    } 
                    else 
                    {
                        JSONCONS_THROW(json_runtime_error<std::invalid_argument>("uri error."));
                    }
                }
            }

            if (!path.empty()) 
            {
                // if the URI is not opaque and the path is not already prefixed
                // with a '/', add one.
                path_.first = uri_string_.length();
                if (!host.empty() && (path.front() != '/')) 
                {
                    uri_string_.push_back('/');
                }
                uri_string_.append(path.data(), path.size());
                path_.second = uri_string_.length();
            }
            else
            {
                path_.first = path_.second = uri_string_.length();
            }

            if (!query.empty()) 
            {
                uri_string_.append("?");
                query_.first = uri_string_.length();
                uri_string_.append(query.data(), query.size());
                query_.second = uri_string_.length();
            }
            else
            {
                query_.first = query_.second = uri_string_.length();
            }

            if (!fragment.empty()) 
            {
                uri_string_.append("#");
                fragment_.first = uri_string_.length();
                uri_string_.append(fragment.data(), fragment.size());
                fragment_.second = uri_string_.length();
            }
            else
            {
                fragment_.first = fragment_.second = uri_string_.length();
            }
        }

        uri& operator=(const uri& other) 
        {
            if (&other != this)
            {
                uri_string_ = other.uri_string_;
                scheme_ = other.scheme_;
                userinfo_ = other.userinfo_;
                host_ = other.host_;
                port_ = other.port_;
                path_ = other.path_;
                query_ = other.query_;
                fragment_ = other.fragment_;
            }
            return *this;
        }

        uri& operator=(uri&& other) noexcept
        {
            if (&other != this)
            {
                uri_string_ = std::move(other.uri_string_);
                scheme_ = other.scheme_;
                userinfo_ = other.userinfo_;
                host_ = other.host_;
                port_ = other.port_;
                path_ = other.path_;
                query_ = other.query_;
                fragment_ = other.fragment_;
            }
            return *this;
        }


        const std::string& string() const noexcept
        {
            return uri_string_;
        }

        bool is_absolute() const noexcept
        {
            return scheme_.second > scheme_.first;
        }

        bool is_opaque() const noexcept 
        {
          return is_absolute() && !raw_authority().empty();
        }

        uri base() const noexcept 
        { 
            return uri{ scheme(), userinfo(), host(), port(), path(), jsoncons::string_view(), jsoncons::string_view()};
        }

        string_view scheme() const noexcept { return string_view(uri_string_.data()+scheme_.first,(scheme_.second-scheme_.first)); }

        std::string userinfo() const 
        {
            return decode_part(raw_userinfo());
        }

        string_view raw_userinfo() const noexcept { return string_view(uri_string_.data()+userinfo_.first,(userinfo_.second-userinfo_.first)); }

        string_view host() const noexcept { return string_view(uri_string_.data()+host_.first,(host_.second-host_.first)); }

        string_view port() const noexcept { return string_view(uri_string_.data()+port_.first,(port_.second-port_.first)); }

        std::string authority() const
        {
            return decode_part(raw_authority());
        }

        string_view raw_authority() const noexcept { return string_view(uri_string_.data()+userinfo_.first,(port_.second-userinfo_.first)); }

        std::string path() const
        {
            return decode_part(raw_path());
        }

        string_view raw_path() const noexcept { return string_view(uri_string_.data()+path_.first,(path_.second-path_.first)); }

        std::string query() const
        {
            return decode_part(raw_query());
        }

        string_view raw_query() const noexcept { return string_view(uri_string_.data()+query_.first,(query_.second-query_.first)); }

        std::string fragment() const
        {
            return decode_part(raw_fragment());
        }

        string_view raw_fragment() const noexcept 
        { 
            return string_view(uri_string_.data()+fragment_.first,(fragment_.second-fragment_.first)); 
        }

        bool has_scheme() const noexcept
        {
            return !scheme().empty();
        }

        bool has_userinfo() const noexcept
        {
            return !raw_userinfo().empty();
        }

        bool has_authority() const noexcept
        {
            return !raw_authority().empty();
        }

        bool has_host() const noexcept
        {
            return !host().empty();
        }

        bool has_port() const noexcept
        {
            return !port().empty();
        }

        bool has_path() const noexcept
        {
            return !raw_path().empty();
        }

        bool has_query() const noexcept
        {
            return !raw_query().empty();
        }

        bool has_fragment() const noexcept
        {
            return !raw_fragment().empty();
        }

        uri resolve(const uri& reference) const
        {
            // This implementation uses the psuedo-code given in
            // http://tools.ietf.org/html/rfc3986#section-5.2.2

            if (reference.is_absolute() && !reference.is_opaque()) 
            {
                return reference;
            }

            if (reference.is_opaque()) 
            {
                return reference;
            }

            std::string userinfo, host, port, path, query, fragment;

            if (reference.has_authority()) 
            {
              // g -> http://g
              if (reference.has_userinfo()) 
              {
                  userinfo = std::string(reference.raw_userinfo());
              }

              if (reference.has_host()) 
              {
                  host = std::string(reference.host());
              }

              if (reference.has_port()) 
              {
                  port = std::string(reference.port());
              }

              if (reference.has_path()) 
              {
                  path = remove_dot_segments(std::string(reference.raw_path()));
              }

              if (reference.has_query()) 
              {
                  query = std::string(reference.raw_query());
              }
            } 
            else 
            {
              if (!reference.has_path()) 
              {
                if (has_path()) 
                {
                    path = std::string(raw_path());
                }

                if (reference.has_query()) 
                {
                    query = std::string(reference.raw_query());
                } 
                else if (has_query()) 
                {
                    query = std::string(raw_query());
                }
              } 
              else 
              {
                  if (reference.raw_path().front() == '/') 
                  {
                    path = remove_dot_segments(std::string(reference.raw_path()));
                  } 
                  else 
                  {
                      path = merge_paths(*this, reference);
                  }

                  if (reference.has_query()) 
                  {
                      query = std::string(reference.raw_query());
                  }
              }

              if (has_userinfo()) 
              {
                  userinfo = std::string(raw_userinfo());
              }

              if (has_host()) 
              {
                  host = std::string(this->host());
              }

              if (has_port()) 
              {
                  port = std::string(this->port());
              }
            }

            if (reference.has_fragment()) 
            {
                fragment = std::string(reference.raw_fragment());
            }

            return uri(uri_raw_parts, std::string(scheme()), userinfo, host, port, path, query, fragment);
        }

        int compare(const uri& other) const
        {
            int result = scheme().compare(other.scheme());
            if (result != 0) return result;
            result = raw_userinfo().compare(other.raw_userinfo());
            if (result != 0) return result;
            result = host().compare(other.host());
            if (result != 0) return result;
            result = port().compare(other.port());
            if (result != 0) return result;
            result = raw_path().compare(other.raw_path());
            if (result != 0) return result;
            result = raw_query().compare(other.raw_query());
            if (result != 0) return result;
            result = raw_fragment().compare(other.raw_fragment());

            return result;
        }

        friend bool operator==(const uri& lhs, const uri& rhs)
        {
            return lhs.compare(rhs) == 0;
        }

        friend bool operator!=(const uri& lhs, const uri& rhs)
        {
            return lhs.compare(rhs) != 0;
        }

        friend bool operator<(const uri& lhs, const uri& rhs)
        {
            return lhs.compare(rhs) < 0;
        }

        friend bool operator<=(const uri& lhs, const uri& rhs)
        {
            return lhs.compare(rhs) <= 0;
        }

        friend bool operator>(const uri& lhs, const uri& rhs)
        {
            return lhs.compare(rhs) > 0;
        }

        friend bool operator>=(const uri& lhs, const uri& rhs)
        {
            return lhs.compare(rhs) >= 0;
        }

        static std::string decode_part(const jsoncons::string_view& encoded)
        {
            std::string decoded;

            std::size_t length = encoded.size();
            for (std::size_t i = 0; i < length;)
            {
                if (encoded[i] == '%' && (length - i) >= 3)
                {
                    auto hex = encoded.substr(i + 1, 2);

                    uint8_t n;
                    jsoncons::detail::hex_to_integer(hex.data(), hex.size(), n);
                    decoded.push_back((char)n);
                    i += 3;
                }
                else
                {
                    decoded.push_back(encoded[i]);
                    ++i;
                }
            }
            return decoded;
        }
        static uri parse(const std::string& str, std::error_code& ec)
        {
            part_type scheme;
            part_type userinfo;
            part_type host;
            part_type port;
            part_type path;
            part_type query;
            part_type fragment;

            std::size_t start = 0;

            parse_state state = parse_state::expect_scheme;
            for (std::size_t i = 0; i < str.size(); ++i)
            {
                char c = str[i];
                switch (state)
                {
                    case parse_state::expect_scheme:
                        switch (c)
                        {
                            case ':':
                                scheme = std::make_pair(start,i);
                                state = parse_state::expect_first_slash;
                                start = i;
                                break;
                            case '?':
                                path = std::make_pair(start, i);
                                state = parse_state::expect_query;
                                start = i + 1;
                                break;
                            case '#':
                                userinfo = std::make_pair(start,start);
                                host = std::make_pair(start,start);
                                port = std::make_pair(start,start);
                                path = std::make_pair(start,i);
                                query = std::make_pair(i,i);
                                state = parse_state::expect_fragment;
                                start = i+1;
                                break;
                            default:
                                break;
                        }
                        break;
                    case parse_state::expect_first_slash:
                        switch (c)
                        {
                            case '/':
                                state = parse_state::expect_second_slash;
                                break;
                            default:
                                start = i;
                                state = parse_state::expect_path;
                                break;
                        }
                        break;
                    case parse_state::expect_second_slash:
                        switch (c)
                        {
                            case '/':
                                state = parse_state::expect_authority;
                                start = i+1;
                                break;
                            default:
                                break;
                        }
                        break;
                    case parse_state::expect_authority:
                        switch (c)
                        {
                            case '[':
                                state = parse_state::expect_host_ipv6;
                                start = i+1;
                                break;
                            default:
                                state = parse_state::expect_userinfo;
                                start = i;
                                --i;
                                break;
                        }
                        break;
                    case parse_state::expect_host_ipv6:
                        switch (c)
                        {
                            case ']':
                                userinfo = std::make_pair(start,start);
                                host = std::make_pair(start,i);
                                port = std::make_pair(i,i);
                                state = parse_state::expect_path;
                                start = i+1;
                                break;
                            default:
                                break;
                        }
                        break;
                    case parse_state::expect_userinfo:
                        switch (c)
                        {
                            case '@':
                                userinfo = std::make_pair(start,i);
                                state = parse_state::expect_host;
                                start = i+1;
                                break;
                            case ':':
                                userinfo = std::make_pair(start,start);
                                host = std::make_pair(start,i);
                                state = parse_state::expect_port;
                                start = i+1;
                                break;
                            case '/':
                                userinfo = std::make_pair(start,start);
                                host = std::make_pair(start,i);
                                port = std::make_pair(i,i);
                                state = parse_state::expect_path;
                                start = i;
                                break;
                            default:
                                break;
                        }
                        break;
                    case parse_state::expect_host:
                        switch (c)
                        {
                            case ':':
                                host = std::make_pair(start,i);
                                state = parse_state::expect_port;
                                start = i+1;
                                break;
                            default:
                                break;
                        }
                        break;
                    case parse_state::expect_port:
                        switch (c)
                        {
                            case '/':
                                port = std::make_pair(start,i);
                                state = parse_state::expect_path;
                                start = i;
                                break;
                            default:
                                break;
                        }
                        break;
                    case parse_state::expect_path:
                        switch (c)
                        {
                            case '?':
                                path = std::make_pair(start,i);
                                state = parse_state::expect_query;
                                start = i+1;
                                break;
                            case '#':
                                path = std::make_pair(start,i);
                                query = std::make_pair(i,i);
                                state = parse_state::expect_fragment;
                                start = i+1;
                                break;
                            default:
                                if (!(is_pchar(c,str.data()+i, str.size() - i) || c == '/'))
                                {
                                    ec = uri_errc::invalid_characters_in_path;
                                    return uri{};
                                }                                
                                break;
                        }
                        break;
                    case parse_state::expect_query:
                        switch (c)
                        {
                            case '#':
                                query = std::make_pair(start,i);
                                state = parse_state::expect_fragment;
                                start = i+1;
                                break;
                            default:
                                break;
                        }
                        break;
                    case parse_state::expect_fragment:
                        break;
                }
            }
            switch (state)
            {
                case parse_state::expect_scheme:
                    userinfo = std::make_pair(start,start);
                    host = std::make_pair(start,start);
                    port = std::make_pair(start,start);
                    path = std::make_pair(start,str.size());
                    query = std::make_pair(str.size(), str.size());
                    fragment = std::make_pair(str.size(), str.size());
                    break;
                case parse_state::expect_userinfo:
                    userinfo = std::make_pair(start,start);
                    host = std::make_pair(start,str.size());
                    port = std::make_pair(str.size(), str.size());
                    path = std::make_pair(str.size(), str.size());
                    query = std::make_pair(str.size(), str.size());
                    fragment = std::make_pair(str.size(), str.size());
                    break;
                case parse_state::expect_path:
                    path = std::make_pair(start,str.size());
                    query = std::make_pair(str.size(), str.size());
                    fragment = std::make_pair(str.size(), str.size());
                    break;
                case parse_state::expect_query:
                    query = std::make_pair(start,str.size());
                    fragment = std::make_pair(str.size(), str.size());
                    break;
                case parse_state::expect_fragment:
                    fragment = std::make_pair(start,str.size());
                    break;
                default:
                    ec = uri_errc::invalid_uri;
                    break;
            }

            return uri(str, scheme, userinfo, host, port, path, query, fragment);
        }

    private:
        enum class parse_state {expect_scheme,
                                expect_first_slash,
                                expect_second_slash,
                                expect_authority,
                                expect_host_ipv6,
                                expect_userinfo,
                                expect_host,
                                expect_port,
                                expect_path,
                                expect_query,
                                expect_fragment};

        uri(const std::string& uri, part_type scheme, part_type userinfo, 
            part_type host, part_type port, part_type path, 
            part_type query, part_type fragment)
            : uri_string_(uri), scheme_(scheme), userinfo_(userinfo), 
              host_(host), port_(port), path_(path), 
              query_(query), fragment_(fragment)
        {
        }

/*
5.2.4.  Remove Dot Segments

   The pseudocode also refers to a "remove_dot_segments" routine for
   interpreting and removing the special "." and ".." complete path
   segments from a referenced path.  This is done after the path is
   extracted from a reference, whether or not the path was relative, in
   order to remove any invalid or extraneous dot-segments prior to
   forming the target URI.  Although there are many ways to accomplish
   this removal process, we describe a simple method using two string
   buffers.

   1.  The input buffer is initialized with the now-appended path
       components and the output buffer is initialized to the empty
       string.

   2.  While the input buffer is not empty, loop as follows:

       A.  If the input buffer begins with a prefix of "../" or "./",
           then remove that prefix from the input buffer; otherwise,

       B.  if the input buffer begins with a prefix of "/./" or "/.",
           where "." is a complete path segment, then replace that
           prefix with "/" in the input buffer; otherwise,

       C.  if the input buffer begins with a prefix of "/../" or "/..",
           where ".." is a complete path segment, then replace that
           prefix with "/" in the input buffer and remove the last
           segment and its preceding "/" (if any) from the output
           buffer; otherwise,

       D.  if the input buffer consists only of "." or "..", then remove
           that from the input buffer; otherwise,

       E.  move the first path segment in the input buffer to the end of
           the output buffer, including the initial "/" character (if
           any) and any subsequent characters up to, but not including,
           the next "/" character or the end of the input buffer.

   3.  Finally, the output buffer is returned as the result of
       remove_dot_segments.
*/

        static std::string remove_dot_segments(std::string input)
        {
            std::string output;
             
            std::size_t rel = 0;
            const std::size_t buflen = input.size();
            while (rel < buflen)
            {
                char* data = &input[0]+rel;
                const std::size_t length = buflen - rel;

                if (length >= 3 && data[0] == '.' && data[1] == '.' && data[2] == '/')
                { 
                    rel += 3;
                }
                else if (length >= 2 && data[0] == '.' && data[1] == '/')
                {
                    rel += 2;
                }
                else if (length >= 3 && data[0] == '/' && data[1] == '.' && data[2] == '/')
                { 
                    rel += 2;
                    data[2] = '/';
                }
                else if (length == 2 && data[0] == '/' && data[1] == '.')
                {
                    ++rel;
                    data[1] = '/';
                }
                else if (length >= 4 && data[0] == '/' && data[1] == '.' && data[2] == '.' && data[3] == '/')
                { 
                    rel += 3;
                    data[3] = '/';
                    auto rslash = output.rfind('/');
                    if (rslash != std::string::npos)
                    {
                        output.erase(rslash);
                    }
                }
                else if (length >= 3 && data[0] == '/' && data[1] == '.' && data[2] == '.')
                { 
                    rel += 2;
                    data[2] = '/';
                    auto rslash = output.rfind('/');
                    if (rslash != std::string::npos)
                    {
                        output.erase(rslash);
                    }
                }
                else if (length == 1 && data[0] == '.')
                {
                    ++rel;
                }
                else if (length == 2 && data[0] == '.' && data[1] == '.')
                {
                    rel += 2;
                }
                else
                {
                    const auto last = data+length;
                    auto it = std::find(data+1, last, '/');
                    if (it != last)
                    {
                        output.append(data, it - data);
                        rel += (it - data);
                    }
                    else
                    {
                        output.append(data, length);
                        rel += length;
                    }
                }
            }

            //std::cout << "path: " << path << ", output: " << output << "\n";
            
            return output;
        }

        static std::string merge_paths(const uri& base, const uri& relative)
        {
            std::string result;
            
            if (!base.raw_authority().empty() && base.raw_path().empty()) 
            {
                result = "/";
                //result.append(relative.raw_path().data(), relative.raw_path().length());
            } 
            else 
            {
                const auto& base_path = base.raw_path();
                auto last_slash = base_path.rfind('/');
                result.append(std::string(base_path.substr(0,last_slash+1)));
            }
            if (!relative.raw_path().empty()) 
            {
                result.append(relative.raw_path().begin(), relative.raw_path().end());
            }
            return remove_dot_segments(std::move(result));
        }

        static bool is_alpha(char ch)
        {
            return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'); 
        }

        static bool is_digit(char ch)
        {
            return (ch >= '0' && ch <= '9'); 
        }

        static bool is_alphanum(char ch)
        {
            return is_alpha(ch) || is_digit(ch); 
        }

        static bool is_unreserved(char ch)
        {
            switch (ch)
            {
                case '_':
                case '-':
                case '!':
                case '.':
                case '~':
                case '\'':
                case '(':
                case ')':
                case '*':
                    return true;
                default:
                    return is_alphanum(ch);
            }
        }

        static bool is_punct(char ch)
        {
            switch (ch)
            {
                case ',':
                case ';':
                case ':':
                case '$':
                case '&':
                case '+':
                case '=':
                    return true;
                default:
                    return false;
            }
        }

        static bool is_reserved(char ch)
        {
            switch (ch)
            { 
                case '?':
                case '/':
                case '[':
                case ']':
                case '@':
                    return true;
                default:
                    return is_punct(ch);
            }
        }

        static bool is_hex(char ch)
        {
            switch(ch)
            {
                case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9': 
                case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                    return true;
                default:
                    return false;
            }
        }

        static bool is_pct_encoded(const char* s, std::size_t length)
        {
            return length < 3 ? false : s[0] == '%' && is_hex(s[1]) && is_hex(s[2]);
        }
        
        // sub-delims    = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
        static bool is_sub_delim(char c)
        {
            switch (c)
            {
                case '!':
                    return true;
                case '$':
                    return true;
                case '&':
                    return true;
                case '\'':
                    return true;
                case '(':
                    return true;
                case ')':
                    return true;
                case '*':
                    return true;
                case '+':
                    return true;
                case ',':
                    return true;
                case ';':
                    return true;
                case '=':
                    return true;
                default:
                    return false;
            }
        }

    public:

        // Any character not in the unreserved, punct or escaped categories, and not equal 
        // to the slash character ('/') or the  commercial-at character ('@'), is quoted.

        static void encode_path(const jsoncons::string_view& sv, std::string& encoded)
        {
            const std::size_t length1 = sv.size() <= 2 ? 0 : sv.size() - 2;

            std::size_t i = 0;
            for (; i < length1; ++i)
            {
                char ch = sv[i];

                switch (ch)
                {
                    case '/':
                    case '@':
                        encoded.push_back(sv[i]);
                        break;
                    default:
                    {
                        bool escaped = is_pct_encoded(sv.data()+i,3);
                        if (!is_unreserved(ch) && !is_punct(ch) && !escaped)
                        {
                            encoded.push_back('%');
                            if (uint8_t(ch) <= 15)
                            {
                                encoded.push_back('0');
                            }
                            jsoncons::detail::integer_to_hex((uint8_t)ch, encoded);
                        }
                        else if (escaped)
                        {
                            encoded.push_back(ch);
                            encoded.push_back(sv[++i]);
                            encoded.push_back(sv[++i]);
                        }
                        else
                        {
                            encoded.push_back(ch);
                        }
                        break;
                    }
                }
            }
 
            const std::size_t length2 = sv.size();
            for (; i < length2; ++i)
            {
                char ch = sv[i];

                switch (ch)
                {
                    case '/':
                    case '@':
                        encoded.push_back(ch);
                        break;
                    default:
                    {
                        if (!is_unreserved(ch) && !is_punct(ch))
                        {
                            encoded.push_back('%');
                            jsoncons::detail::integer_to_hex((uint8_t)ch, encoded);
                        }
                        else
                        {
                            encoded.push_back(ch);
                        }
                        break;
                    }
                }
            }
        }


        // Any character not in the unreserved, punct, or escaped categories is quoted.

        static void encode_userinfo(const jsoncons::string_view& sv, std::string& encoded)
        {
            const std::size_t length1 = sv.size() <= 2 ? 0 : sv.size() - 2;

            std::size_t i = 0;
            for (; i < length1; ++i)
            {
                char ch = sv[i];

                bool escaped = is_pct_encoded(sv.data()+i,3);
                if (!is_unreserved(ch) && !is_punct(ch) && !escaped)
                {
                    encoded.push_back('%');
                    if (uint8_t(ch) <= 15)
                    {
                        encoded.push_back('0');
                    }
                    jsoncons::detail::integer_to_hex((uint8_t)ch, encoded);
                }
                else if (escaped)
                {
                    encoded.push_back(ch);
                    encoded.push_back(sv[++i]);
                    encoded.push_back(sv[++i]);
                }
                else
                {
                    encoded.push_back(ch);
                }
            }
 
            const std::size_t length2 = sv.size();
            for (; i < length2; ++i)
            {
                char ch = sv[i];

                if (!is_unreserved(ch) && !is_punct(ch))
                {
                    encoded.push_back('%');
                    jsoncons::detail::integer_to_hex((uint8_t)ch, encoded);
                }
                else
                {
                    encoded.push_back(ch);
                }
            }
        }

        // The set of all legal URI characters consists of the unreserved, reserved, escaped characters.

        static void encode_illegal_characters(const jsoncons::string_view& sv, std::string& encoded)
        {
            const std::size_t length1 = sv.size() <= 2 ? 0 : sv.size() - 2;

            std::size_t i = 0;
            for (; i < length1; ++i)
            {
                char ch = sv[i];

                bool escaped = is_pct_encoded(sv.data()+i,3);
                if (!is_unreserved(ch) && !is_reserved(ch) && !escaped)
                {
                    encoded.push_back('%');
                    if (uint8_t(ch) <= 15)
                    {
                        encoded.push_back('0');
                    }
                    jsoncons::detail::integer_to_hex((uint8_t)ch, encoded);
                }
                else if (escaped)
                {
                    encoded.push_back(ch);
                    encoded.push_back(sv[++i]);
                    encoded.push_back(sv[++i]);
                }
                else
                {
                    encoded.push_back(ch);
                }
            }
 
            const std::size_t length2 = sv.size();
            for (; i < length2; ++i)
            {
                char ch = sv[i];

                if (!is_unreserved(ch) && !is_reserved(ch))
                {
                    encoded.push_back('%');
                    jsoncons::detail::integer_to_hex((uint8_t)ch, encoded);
                }
                else
                {
                    encoded.push_back(ch);
                }
            }
        }

        // rel_segment   = 1*( unreserved | escaped | ";" | "@" | "&" | "=" | "+" | "$" | "," )
        static bool is_rel_segment(char c, const char* s, std::size_t length)
        {
            return is_unreserved(c) || is_pct_encoded(s,length) || c == ';' || c == '@' || c == '&' || c == '=' || c == '+' || c == '$' || c == ',';
        }

        // userinfo      = *( unreserved | escaped | ";" | ":" | "&" | "=" | "+" | "$" | "," )

        static bool is_userinfo(char c, const char* s, std::size_t length)
        {
            return is_unreserved(c) || is_pct_encoded(s,length) || c == ';' || c == ':' || c == '&' || c == '=' || c == '+' || c == '$' || c == ',';
        }

        static bool is_pchar(char c, const char* s, std::size_t length)
        {
            return is_unreserved(c) || is_pct_encoded(s,length) || is_sub_delim(c) || c == ':' || c == '@';
        }
    };

} // namespace jsoncons

#endif
