#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include "core/util.hpp"

namespace file{

    std::vector<uint8_t> readAsBinary(const std::string& filename);
    std::string readAsString(const std::string& filename);

    bool exists(const std::string& filename);

    long size(const std::string& filename);

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

            const jval& operator[](size_t index) const {
                return *this->read<jarray>()[index];
            }

            const jval& operator[](const std::string& tag) const {
                return *this->read<jobj>().at(tag);
            }

            jval& operator[](size_t index) {
                return *this->read<jarray>()[index];
            }

            jval& operator[](const std::string& tag) {
                return *this->read<jobj>().at(tag);
            }

            template <typename T>
            requires isjprimitive<T>
            operator T() {
                return this->read<T>();
            }

            template <typename T>
            requires isjprimitive<T>
            operator T() const {
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
