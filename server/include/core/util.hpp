#include <concepts>
#include <charconv>
#include <string>
#include <typeindex>
#include <type_traits>
#include <memory>
#include <utility>
#include <new>
#include <cstdint>
#include <vector>
#include <unordered_map>

namespace util{

    template <class value_type, int32_t first_index = 0>
    class sequential_unordered_map {
        int32_t counter_ = first_index;
        std::unordered_map<int32_t, value_type> data_{};
    public:
        const std::unordered_map<int32_t, value_type>& unordered_map() {
            return data_;
        }
        int32_t current_id() {
            return counter_ - 1;
        }
        const value_type& operator[](int32_t hash) const {
            return data_.at(hash);
        }
        const value_type* find(int32_t key) const {
            auto it = data_.find(key);
            if (it != data_.end()) {
                return &(it->second);
            }
            return nullptr;
        }
        value_type* find(int32_t key) {
            auto it = data_.find(key);
            if (it != data_.end()) {
                return &(it->second);
            }
            return nullptr;
        }
        void erase(int32_t key) {
            data_.erase(key);
        }
        int32_t push(const value_type& value) {
            int32_t key = counter_++;
            data_.insert({key, value});
            return key;
        }
        int32_t push(value_type&& value) {
            int32_t key = counter_++;
            data_.insert({key, std::move(value)});
            return key;
        }
    };

    template <typename t_data>
    requires (!std::is_convertible_v<t_data, int>)
    struct uresult : public t_data
    {
        uresult(bool s, t_data data)
        {
            success = s;
            t_data& d = *this;
            d = data;
        }
        uresult(bool s)
        {
            success = s;
        }
        uresult(t_data data)
        {
            success = true;
            t_data& d = *this;
            d = data;
        }
        bool success = false;
        explicit operator bool() { return success; }
        operator t_data&() { return *static_cast<t_data*>(this); }
    };

    template <typename T>
    requires std::same_as<T, float> or std::same_as<T, double>
    std::string tochars(T value)
    {
        char buffer[64];
        auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value);
        return std::string(buffer, ptr - buffer);
    }

    template <typename... args>
    constexpr uint32_t getlargestsize()
    {
        uint32_t largest = 0;
        ((largest = sizeof(args) > largest ? sizeof(args) : largest), ...);
        return largest;
    }

    template <typename... args>
    constexpr uint32_t getlargestalign()
    {
        uint32_t largest = 0;
        ((largest = alignof(args) > largest ? alignof(args) : largest), ...);
        return largest;
    }

    template <typename... args>
    struct pun
    {
    private:
        alignas(getlargestalign<args...>()) uint8_t puffer_[getlargestsize<args...>()]; // nice pun hehe>????
        std::type_index rtti_ = typeid(void);

        template <typename func>
        void type_(func fun) const
        {
            ([&]{
                if (this->is<args>())
                {
                    args* dummy{nullptr};
                    fun(dummy);
                }
            }(), ...);
        }

        void destruct_()
        {
            type_([&](auto* a){ using t = std::remove_pointer_t<decltype(a)>;
                std::destroy_at(reinterpret_cast<t*>(puffer_));
                rtti_ = typeid(void);
            });
        }

        template <typename T>
        requires ( (std::same_as<T, args> or ...) )
        void copyconstruct_(const T& data)
        {
            std::construct_at(reinterpret_cast<T*>(puffer_), data);
            rtti_ = typeid(T);
        }

        template <typename T>
        requires ( (std::same_as<T, args> or ...) )
        void moveconstruct_(T&& data)
        {
            std::construct_at(reinterpret_cast<T*>(puffer_), std::move(data));
            rtti_ = typeid(T);
        }

    public:

        pun() = default;

        pun(const pun& other)
        {
            *this = other;
        }
        pun(pun&& other)
        {
            *this = std::move(other);
        }

        pun& operator=(const pun& other)
        {
            if (this == &other) return *this;
            destruct_();
            other.type_([&](auto* a){ using t = std::remove_pointer_t<decltype(a)>;
                this->copyconstruct_(other.read<t>());
            });
            return *this;
        }

        pun& operator=(pun&& other)
        {
            if (this == &other) return *this;
            destruct_();
            other.type_([&](auto* a){ using t = std::remove_pointer_t<decltype(a)>;
                this->moveconstruct_(std::move(other.read<t>()));
            });
            other.destruct_();
            return *this;
        }

        ~pun()
        {
            destruct_();
        }

        template <typename T>
        requires ( (std::same_as<T, args> or ...) )
        pun(const T& other)
        {
            copyconstruct_(other);
        }

        template <typename T>
        requires ( (std::same_as<T, args> or ...) )
        pun(T&& other)
        {
            moveconstruct_(std::move(other));
        }

        template <typename T>
        requires ( (std::same_as<T, args> or ...) )
        pun& operator=(const T& other)
        {
            this->write(other);
            return *this;
        }

        template <typename T>
        requires ( (std::same_as<T, args> or ...) )
        pun& operator=(T&& other)
        {
            this->write(std::move(other));
            return *this;
        }

        template <typename T>
        requires ( (std::same_as<T, args> or ...) )
        explicit operator T()
        {
            return this->read<T>();
        }

        template <typename T>
        requires ( (std::same_as<T, args> or ...) )
        void write(const T& data)
        {
            destruct_();
            copyconstruct_(data);
        }

        template <typename T>
        requires ( (std::same_as<T, args> or ...) )
        void write(T&& data)
        {
            destruct_();
            moveconstruct_(std::move(data));
        }

        template <typename T>
        requires ( (std::same_as<T, args> or ...) )
        [[nodiscard]]
        T& read()
        {
            OS_ASSERT(this->is<T>(), "pun read type must be equal to that of the last pun write type");
            return *std::launder(reinterpret_cast<T*>(puffer_));
        }

        template <typename T>
        requires ( (std::same_as<T, args> or ...) )
        [[nodiscard]]
        const T& read() const
        {
            OS_ASSERT(this->is<T>(), "pun read type must be equal to that of the last pun write type");
            return *std::launder(reinterpret_cast<const T*>(puffer_));
        }

        template <typename T>
        requires ( (std::same_as<T, args> or ...) )
        [[nodiscard]]
        const bool is() const
        {
            return (rtti_ == std::type_index{typeid(T)});
        }

        template <typename T, typename func>
        requires ((std::same_as<T, args> or ...)) &&
        (std::is_invocable_v<func, T&>)
        void visit(func fun)
        {
            if (is<T>())
            {
                fun(read<T>());
            }
        }

        template <typename T, typename func>
        requires ( (std::same_as<T, args> or ...) ) &&
        (std::is_invocable_v<func, const T&>)
        void visit(func fun) const
        {
            if (is<T>())
            {
                fun(read<T>());
            }
        }
    };

    std::vector<std::string_view> strsplit(std::string_view str, const char splitter);

    struct stoi32result
    {
        int32_t value = 0;
        size_t charLength = 0;
    };

    uresult<stoi32result> stoi32(std::string_view str);
    uresult<std::vector<uresult<stoi32result>>> stoi32vec(std::string_view str, const char splitter, uint32_t veclengthlimit);

    struct stofresult
    {
        float value = 0;
        size_t charLength = 0;
    };

    uresult<stofresult> stof(std::string_view str);
    uresult<std::vector<uresult<stofresult>>> stofvec(std::string_view str, const char splitter, uint32_t veclengthlimit);

    struct stodresult
    {
        double value = 0;
        size_t charLength = 0;
    };

    uresult<stodresult> stod(std::string_view str);
    uresult<std::vector<uresult<stodresult>>> stodvec(std::string_view str, const char splitter, uint32_t veclengthlimit);
}
