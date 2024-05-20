// Copyright 2013-2023 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSONSCHEMA_COMMON_KEYWORD_VALIDATOR_HPP
#define JSONCONS_JSONSCHEMA_COMMON_KEYWORD_VALIDATOR_HPP

#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/uri.hpp>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpointer/jsonpointer.hpp>
#include <jsoncons_ext/jsonschema/jsonschema_error.hpp>
#include <unordered_set>

namespace jsoncons {
namespace jsonschema {

    // Interface for validation error handlers
    class error_reporter
    {
        bool fail_early_;
        std::size_t error_count_;
    public:
        error_reporter(bool fail_early = false)
            : fail_early_(fail_early), error_count_(0)
        {
        }

        virtual ~error_reporter() = default;

        void error(const validation_output& o)
        {
            ++error_count_;
            do_error(o);
        }

        std::size_t error_count() const
        {
            return error_count_;
        }

        bool fail_early() const
        {
            return fail_early_;
        }

    private:
        virtual void do_error(const validation_output& /* e */) = 0;
    };

    template <class Json>
    class validator_base 
    {
    public:
        virtual ~validator_base() = default;

        virtual const std::string& schema_path() const = 0;

        void validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter, 
            Json& patch) const 
        {
            do_validate(instance, instance_location, evaluated_properties, reporter, patch);
        }

    private:
        virtual void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter, 
            Json& patch) const = 0;
    };

    template <class Json>
    class keyword_validator : public validator_base<Json> 
    {
    public:
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        virtual keyword_validator_type clone() const = 0;
    };

    template <class Json>
    class keyword_validator_base : public keyword_validator<Json>
    {
        std::string schema_path_;
    public:
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        keyword_validator_base(const std::string& schema_path)
            : schema_path_(schema_path)
        {
        }

        keyword_validator_base(const keyword_validator_base&) = delete;
        keyword_validator_base(keyword_validator_base&&) = default;
        keyword_validator_base& operator=(const keyword_validator_base&) = delete;
        keyword_validator_base& operator=(keyword_validator_base&&) = default;

        const std::string& schema_path() const override
        {
            return schema_path_;
        }
    };

    template <class Json>
    using uri_resolver = std::function<Json(const jsoncons::uri & /*id*/)>;

    template <class Json>
    class schema_validator : public validator_base<Json>
    {
    public:
        using schema_validator_type = typename std::unique_ptr<schema_validator<Json>>;
        using keyword_validator_type = typename std::unique_ptr<keyword_validator<Json>>;

    public:
        schema_validator()
        {}

        virtual jsoncons::optional<Json> get_default_value() const = 0;

        virtual schema_validator_type clone() const = 0;
    };

    template <class Json>
    class ref_validator : public keyword_validator<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;
        using schema_validator_type = std::unique_ptr<schema_validator<Json>>;

        schema_validator_type referred_schema_;

    public:
        ref_validator(const std::string& /*id*/)
        {}

        ref_validator(schema_validator_type&& target)
            : referred_schema_(std::move(target)) {}

        void set_referred_schema(schema_validator_type&& target) { referred_schema_ = std::move(target); }

        const std::string& schema_path() const override
        {
            static std::string s = "#";
            return referred_schema_ ? referred_schema_->schema_path() : s;
        }

        keyword_validator_type clone() const override 
        {
            schema_validator_type referred_schema;
            if (referred_schema_)
            {
                referred_schema = referred_schema_->clone();
            }

            return jsoncons::make_unique<ref_validator>(std::move(referred_schema));
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter, 
            Json& patch) const override
        {
            if (!referred_schema_)
            {
                reporter.error(validation_output("", 
                                                 this->schema_path(), 
                                                 instance_location.to_uri_fragment(), 
                                                 "Unresolved schema reference " + this->schema_path()));
                return;
            }

            referred_schema_->validate(instance, instance_location, evaluated_properties, reporter, patch);
        }
    };

    template <class Json>
    class boolean_schema_validator : public schema_validator<Json>
    {
    public:
        using schema_validator_type = typename std::unique_ptr<schema_validator<Json>>;
        using keyword_validator_type = typename std::unique_ptr<keyword_validator<Json>>;

        std::string schema_path_;
        bool value_;

    public:
        boolean_schema_validator(const std::string& schema_path, bool value)
            : schema_path_(schema_path), value_(value)
        {
        }

        jsoncons::optional<Json> get_default_value() const override
        {
            return jsoncons::optional<Json>{};
        }

        const std::string& schema_path() const override
        {
            return schema_path_;
        }

        schema_validator_type clone() const final
        {
            return jsoncons::make_unique<boolean_schema_validator>(schema_path_, value_);
        }

    private:
        void do_validate(const Json&, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter, 
            Json&) const final
        {
            if (!value_)
            {
                reporter.error(validation_output("false", 
                    this->schema_path(), 
                    instance_location.to_uri_fragment(), 
                    "False schema always fails"));
            }
        }
    };

    template <class Json>
    class object_schema_validator : public schema_validator<Json>
    {
    public:
        using schema_validator_type = typename std::unique_ptr<schema_validator<Json>>;
        using keyword_validator_type = typename std::unique_ptr<keyword_validator<Json>>;

        std::string schema_path_;
        std::vector<keyword_validator_type> validators_;
        Json default_value_;

    public:
        object_schema_validator(const std::string& schema_path, std::vector<keyword_validator_type>&& validators, Json&& default_value)
            : schema_path_(schema_path),
              validators_(std::move(validators)),
              default_value_(std::move(default_value))
        {
        }

        jsoncons::optional<Json> get_default_value() const override
        {
            return default_value_;
        }

        const std::string& schema_path() const override
        {
            return schema_path_;
        }

        schema_validator_type clone() const final
        {
            std::vector<keyword_validator_type> validators;
            for (auto& validator : validators_)
            {
                validators.push_back(validator->clone());
            }

            return jsoncons::make_unique<object_schema_validator>(schema_path_, std::move(validators), Json(default_value_));
        }

    private:
        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter, 
            Json& patch) const final
        {
            std::unordered_set<std::string> local_evaluated_properties;

            for (auto& validator : validators_)
            {
                validator->validate(instance, instance_location, local_evaluated_properties, reporter, patch);
                if (reporter.error_count() > 0 && reporter.fail_early())
                {
                    return;
                }
            }

            for (auto&& name : local_evaluated_properties)
            {
                evaluated_properties.emplace(std::move(name));
            }
        }
    };

    // keyword_validator_wrapper

    template <class Json>
    class keyword_validator_wrapper : public keyword_validator<Json>
    {
        using keyword_validator_type = typename keyword_validator<Json>::keyword_validator_type;
        const keyword_validator<Json>* validator_;
    public:
        keyword_validator_wrapper(const keyword_validator<Json>* validator)
            : validator_(validator)
        {
        }

        const std::string& schema_path() const override
        {
            static std::string s("#");
            return validator_ != nullptr ? validator_->schema_path() : s;
        }

        keyword_validator_type clone() const final 
        {
            return jsoncons::make_unique<keyword_validator_wrapper>(validator_);
        }

    private:
        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location, 
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter,
            Json& patch) const override
        {
            validator_->validate(instance, instance_location, evaluated_properties, reporter, patch);
        }
    };

} // namespace jsonschema
} // namespace jsoncons

#endif // JSONCONS_JSONSCHEMA_KEYWORD_VALIDATOR_HPP
