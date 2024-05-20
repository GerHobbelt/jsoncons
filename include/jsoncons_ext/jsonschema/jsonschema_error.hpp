/// Copyright 2013-2024 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version
 
#ifndef JSONCONS_JSONSCHEMA_JSONSCHEMA_ERROR_HPP
#define JSONCONS_JSONSCHEMA_JSONSCHEMA_ERROR_HPP

#include <jsoncons/json_exception.hpp>
#include <system_error>

namespace jsoncons {
namespace jsonschema {

    class schema_error : public std::runtime_error, public virtual json_exception
    {
    public:
        schema_error(const std::string& message)
            : std::runtime_error(message)
        {
        }

        const char* what() const noexcept override
        {
            return std::runtime_error::what();
        }  
    };

    class validation_error : public std::runtime_error, public virtual json_exception
    {
    public:
        validation_error(const std::string& message)
            : std::runtime_error(message)
        {
        }

        const char* what() const noexcept override
        {
            return std::runtime_error::what();
        }  
    };

    class validation_message 
    {
        std::string keyword_;
        jsonpointer::json_pointer eval_path_;
        uri schema_path_;
        jsonpointer::json_pointer instance_location_;
        std::string message_;
        std::vector<validation_message> details_;
    public:
        validation_message(std::string keyword,
            jsonpointer::json_pointer eval_path,
            uri schema_path,
            jsonpointer::json_pointer instance_location,
            std::string message)
            : keyword_(std::move(keyword)), 
              eval_path_(std::move(eval_path)),
              schema_path_(std::move(schema_path)),
              instance_location_(std::move(instance_location)),
              message_(std::move(message))
        {
        }

        validation_message(const std::string& keyword,
            const jsonpointer::json_pointer& eval_path,
            const uri& schema_path,
            const jsonpointer::json_pointer& instance_location,
            const std::string& message,
            const std::vector<validation_message>& details)
            : keyword_(keyword),
              eval_path_(eval_path),
              schema_path_(schema_path),
              instance_location_(instance_location), 
              message_(message),
              details_(details)
        {
        }

        const jsonpointer::json_pointer& instance_location() const
        {
            return instance_location_;
        }

        const std::string& message() const
        {
            return message_;
        }

        const jsonpointer::json_pointer& eval_path() const
        {
            return eval_path_;
        }

        const uri& schema_path() const
        {
            return schema_path_;
        }

        const std::string keyword_location() const
        {
            return eval_path_.to_string();
        }

        const std::string absolute_keyword_location() const
        {
            return schema_path_.string();
        }

        const std::string& keyword() const
        {
            return keyword_;
        }

        const std::vector<validation_message>& details() const
        {
            return details_;
        }
    };

} // namespace jsonschema
} // namespace jsoncons

#endif // JSONCONS_JSONSCHEMA_JSONSCHEMA_ERROR_HPP
