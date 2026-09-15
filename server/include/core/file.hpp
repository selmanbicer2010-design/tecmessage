#pragma once

#include <cstdint>
#include <string_view>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include "core/util.hpp"

namespace file{

    void ferr(std::string_view path, int32_t error_type, int32_t* err, std::source_location location = std::source_location::current());

    std::vector<uint8_t> fread(std::string_view path, int32_t* err = nullptr);
    bool exists(std::string_view path, int32_t* err = nullptr);
    uint64_t size(std::string_view path, int32_t* err = nullptr);

    namespace json
    {
        struct jval;
        using uptrjval = std::unique_ptr<jval>;
        struct jarray : public std::vector<uptrjval>
        {
            jarray() = default;
            jarray(const jarray& other);
            jarray(jarray&& other);
            jarray& operator=(const jarray& other);
            jarray& operator=(jarray&& other);
        };
        struct jobj : public std::unordered_map<std::string, uptrjval>
        {
            jobj() = default;
            jobj(const jobj& other);
            jobj(jobj&& other);
            jobj& operator=(const jobj& other);
            jobj& operator=(jobj&& other);
            bool has(const std::string& key) const {
                return this->count(key) > 0;
            }

        };

        template <typename T>
        concept isjprimitive = (
            std::same_as<T, std::nullptr_t> or std::same_as<T, bool> or
            std::same_as<T, double> or std::same_as<T, std::string>);

        template <typename T>
        concept isjvaltype = (
            isjprimitive<T> or
            std::same_as<T, jarray> or std::same_as<T, jobj>
            );

        struct jval : public util::pun<std::nullptr_t, bool, double, std::string, jarray, jobj>
        {
            using pun::pun;

            [[nodiscard]]
            const jval& operator[](size_t index) const {
                return *this->read<jarray>()[index];
            }

            [[nodiscard]]
            const jval& operator[](const std::string& tag) const {
                return *this->read<jobj>().at(tag);
            }

            [[nodiscard]]
            jval& operator[](size_t index) {
                return *this->read<jarray>()[index];
            }

            [[nodiscard]]
            jval& operator[](const std::string& tag) {
                return *this->read<jobj>().at(tag);
            }

            bool is_object() const {
                return this->is<jobj>();
            }
            bool is_array() const {
                return this->is<jarray>();
            }

            bool has(const std::string& tag) const {
                return this->read<jobj>().count(tag) > 0;
            }

            template <typename T>
            requires isjvaltype<T>
            operator T&() {
                return this->read<T>();
            }

            template <typename T>
            requires isjvaltype<T>
            operator T&() const {
                return this->read<T>();
            }

        };

        template <typename T>
        requires isjprimitive<T>
        uptrjval makejprimitive(const T& v)
        { return std::make_unique<file::json::jval>(v); }
        template <typename T>
        requires std::same_as<T, jval>
        uptrjval copyjval(const T& v)
        { return std::make_unique<file::json::jval>(v); }
        template <typename T>
        requires std::same_as<T, jarray> or std::same_as<T, jobj>
        uptrjval wrapjval(T&& v)
        { return std::make_unique<file::json::jval>(std::forward<T>(v)); }
        uptrjval makejarray(const std::initializer_list<jval>& initlist);
        uptrjval makejobj(const std::initializer_list<std::pair<std::string, jval>>& initlist);

        struct tree
        {
            jval root;
        };

        [[nodiscard("We're doing heavy work ere shun! Ya bedda pick up on dis!")]]
        tree deserialize(const std::string& path);
        [[nodiscard]]
        tree parse(const std::string& src);
        std::string serialize(const tree& tree);

    }
}
