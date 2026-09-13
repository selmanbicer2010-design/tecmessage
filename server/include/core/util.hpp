#pragma once

#include <concepts>
#include <charconv>
#include <source_location>
#include <string>
#include <typeindex>
#include <type_traits>
#include <memory>
#include <utility>
#include <new>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <cassert>
#include "core/enum.hpp"

namespace util{

    template <typename ret_ty = std::string>
    requires std::is_constructible_v<ret_ty, const char*> or std::same_as<ret_ty, const char*>
    auto this_function(std::source_location location = std::source_location::current()) {
        if constexpr (std::same_as<ret_ty, const char*>) {
            return location.function_name();
        }
        return ret_ty{location.function_name()};
    }

    template <typename value_ty>
    class unordered_vector {
    public:
        using value_type = value_ty;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = value_type&;
        using const_reference = const value_type&;
        using iterator = std::vector<value_type>::iterator;
        using const_iterator = std::vector<value_type>::const_iterator;
        using reverse_iterator = std::vector<value_type>::reverse_iterator;
        using const_reverse_iterator = std::vector<value_type>::const_reverse_iterator;
        [[nodiscard]] iterator begin() { return c.begin(); }
        [[nodiscard]] const_iterator begin() const { return c.begin(); }
        [[nodiscard]] const_iterator cbegin() const { return c.cbegin(); }

        [[nodiscard]] iterator end() { return c.end(); }
        [[nodiscard]] const_iterator end() const { return c.end(); }
        [[nodiscard]] const_iterator cend() const { return c.cend(); }

        [[nodiscard]] reverse_iterator rbegin() { return c.rbegin(); }
        [[nodiscard]] const_reverse_iterator rbegin() const { return c.rbegin(); }
        [[nodiscard]] const_reverse_iterator crbegin() const { return c.crbegin(); }

        [[nodiscard]] reverse_iterator rend() { return c.rend(); }
        [[nodiscard]] const_reverse_iterator rend() const { return c.rend(); }
        [[nodiscard]] const_reverse_iterator crend() const { return c.crend(); }

        [[nodiscard]] size_type data_size() const { return c.size(); }
        [[nodiscard]] size_type map_size() const { return a.size(); }
        [[nodiscard]] bool empty() const { return c.empty(); }

        [[nodiscard]] value_type* data() { return c.data(); }
        [[nodiscard]] const value_type* data() const { return c.data(); }

        void clear() { a.clear(); b.clear(); c.clear(); }

        static constexpr int32_t invalid_index = -1;
        std::vector<int32_t> a;
        std::vector<int32_t> b;
        std::vector<value_type> c;

        [[nodiscard]] reference operator[](size_type i) { return c[a[i]]; }
        [[nodiscard]] const_reference operator[](size_type i) const { return c[a[i]]; }

        int32_t next_handle() const { return static_cast<int32_t>(a.size()); }

        int32_t push_back(const value_type& value) {
            int32_t handle = static_cast<int32_t>(a.size());
            a.push_back(static_cast<int32_t>(c.size()));
            b.push_back(handle);
            c.push_back(value);
            return handle;
        }

        int32_t push_back(value_type&& value) {
            int32_t handle = static_cast<int32_t>(a.size());
            a.push_back(static_cast<int32_t>(c.size()));
            b.push_back(handle);
            c.push_back(std::move(value));
            return handle;
        }

        value_type* find(int32_t i) {
            if (a[i] == invalid_index) return nullptr;
            return &(c[a[i]]);
        }

        value_type* at(int32_t i) {
            if (i < 0 || static_cast<size_type>(i) >= a.size() or a[i] == invalid_index) return nullptr;
            return &(c[a[i]]);
        }

        void erase(int32_t i) {
            int32_t ai = a[i];
            int32_t bback = b.back();
            c[ai] = std::move(c.back());
            c.pop_back();
            b[ai] = bback;
            b.pop_back();
            a[bback] = ai;
            a[i] = invalid_index;
        }
    };

    template <class value_type>
    class sequential_unordered_map {
    public:
        static constexpr int32_t counter_min = 0;
        static constexpr int32_t invalid_key = counter_min - 1;
    private:
        int32_t counter_ = counter_min;
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
        [[nodiscard("value is left dangling without taking key")]]
        int32_t push(const value_type& value) {
            int32_t key = counter_++;
            data_.insert({key, value});
            return key;
        }
        [[nodiscard("value is left dangling without taking key")]]
        int32_t push(value_type&& value) {
            int32_t key = counter_++;
            data_.emplace(key, std::move(value));
            return key;
        }
    };

    template <typename value_type>
    class unique_val {
    private:
        sequential_unordered_map<value_type>* owner_p{nullptr};
        int32_t key_v{sequential_unordered_map<value_type>::invalid_key};
        bool destroyed = false;
        void erase_if_valid_() {
            if (destroyed) return;
            destroyed = true;
            if (has_owner() and has_key()) {
                owner_p->erase(key_v);
                key_v = sequential_unordered_map<value_type>::invalid_key;
            }
        }
    public:
        unique_val() = default;
        unique_val(sequential_unordered_map<value_type>& owner) : owner_p(&owner), key_v(owner.push(value_type{})) {}
        //unique_val(sequential_unordered_map<value_type>& owner, int32_t key) : owner_p(&owner), key(key) {}
        unique_val(const unique_val& other) : owner_p(other.owner_p), key_v(other.has_owner() ? other.owner().push(value_type{other.get()}) : sequential_unordered_map<value_type>::invalid_key) {}
        unique_val(unique_val&& other) : owner_p(other.owner_p), key_v(other.key_v) { other.owner_p = nullptr; other.key_v = sequential_unordered_map<value_type>::invalid_key; }
        unique_val& operator=(const unique_val& other) {
            if (this == &other) return *this;
            erase_if_valid_();
            this->owner_p = other.owner_p;
            key_v = owner().push(value_type{other.get()});
            return *this;
        }
        unique_val& operator=(unique_val&& other) {
            if (this == &other) return *this;
            erase_if_valid_();
            this->owner_p = other.owner_p;
            this->key_v = other.key_v;
            other.owner_p = nullptr;
            other.key_v = sequential_unordered_map<value_type>::invalid_key;
            return *this;
        }
        ~unique_val() {
            erase_if_valid_();
        }
        void destroy() {
            erase_if_valid_();
        }
        bool has_owner() const {
            return owner_p != nullptr;
        }
        bool has_key() const {
            if (has_owner()) {
                return owner().unordered_map().contains(key_v) and key_v >= sequential_unordered_map<value_type>::counter_min;
            }
            return false;
        }
        sequential_unordered_map<value_type>& owner() const {
            return *owner_p;
        }
        int32_t key() {
            return key_v;
        }
        value_type& get() const {
            return *owner().find(key_v);
        }
        value_type* operator->() const {
            return &get();
        }
        void move_into_new_owner(sequential_unordered_map<value_type>& newowner) {
            value_type old = std::move(get());
            owner().erase(key_v);
            owner_p = &newowner;
            key_v = newowner.push(std::move(old));
        }
    };

    template <typename value_type>
    class shared_val;

    template <typename value_type>
    class handle_val {
        friend class shared_val<value_type>;
    private:
        sequential_unordered_map<value_type>* owner_p{nullptr};
        int32_t key_v{0};
    public:
        handle_val() = default;
        handle_val(sequential_unordered_map<value_type>& owner, int32_t key) : owner_p(&owner), key_v(key) {}
        handle_val(const handle_val& other) : owner_p(other.owner_p), key_v(other.key_v) {}
        handle_val(handle_val&& other) : owner_p(other.owner_p), key_v(other.key_v) {}
        handle_val& operator=(const handle_val& other) {
            if (this == &other) return *this;
            this->owner_p = other.owner_p;
            this->key_v = other.key_v;
            return *this;
        }
        handle_val& operator=(handle_val&& other) {
            if (this == &other) return *this;
            this->owner_p = other.owner_p;
            this->key_v = other.key_v;
            return *this;
        }
        bool has_owner() const {
            return owner_p != nullptr;
        }
        bool has_key() const {
            if (has_owner()) {
                return owner().unordered_map().contains(key_v) and key_v >= sequential_unordered_map<value_type>::counter_min;
            }
            return false;
        }
        sequential_unordered_map<value_type>& owner() const {
            return *owner_p;
        }
        int32_t key() {
            return key_v;
        }
        value_type& get() const {
            return *owner().find(key_v);
        }
        value_type* operator->() const {
            return &get();
        }
    };

    template <typename value_type>
    class seq_control_block {
    public:
        sequential_unordered_map<value_type> values;
        int32_t references = 0;
    };

    template <typename value_type> class shared_val;
    template <typename value_type> shared_val<value_type> create_shared_val();

    template <typename value_type>
    class shared_val {
        friend shared_val<value_type> create_shared_val<value_type>();
    private:
        seq_control_block<value_type>* control_ = nullptr;
        handle_val<value_type> handle_{};
        shared_val(seq_control_block<value_type>* control) : control_(control) {
            if (control_) {
                handle_ = handle_val<value_type>(control_->values, control_->values.push(value_type{}));
                control_->references++;
            }
        }
        void release_() {
            if (control_) {
                control_->references--;
                if (control_->references == 0) {
                    delete control_;
                }
            }
        }
    public:
        shared_val() = delete;
        shared_val(const shared_val& other) {
            this->control_ = other.control_;
            this->handle_ = other.handle_;
            if (control_) control_->references++;
        }
        shared_val(shared_val&& other) {
            this->control_ = other.control_;
            this->handle_ = other.handle_;
            other.control_ = nullptr;
        }
        shared_val& operator=(const shared_val& other) {
            if (this == &other) return *this;
            release_();
            this->control_ = other.control_;
            this->handle_ = other.handle_;
            if (control_) control_->references++;
            return *this;
        }
        shared_val& operator=(shared_val&& other) {
            if (this == &other) return *this;
            release_();
            this->control_ = other.control_;
            this->handle_ = other.handle_;
            other.control_ = nullptr;
            return *this;
        }
        ~shared_val() {
            release_();
        }
        value_type& get() {
            return handle_.get();
        }
        value_type* operator->() const {
            return &get();
        }
    };

    template <typename value_type>
    shared_val<value_type> create_shared_val() {
        auto control = new seq_control_block<value_type>();
        return shared_val<value_type>(control);
    }

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
            LOG_ASSERT(this->is<T>(), "pun read type must be equal to that of the last pun write type");
            return *std::launder(reinterpret_cast<T*>(puffer_));
        }

        template <typename T>
        requires ( (std::same_as<T, args> or ...) )
        [[nodiscard]]
        const T& read() const
        {
            LOG_ASSERT(this->is<T>(), "pun read type must be equal to that of the last pun write type");
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
