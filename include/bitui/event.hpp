#pragma once

#include <cstdint>
#include <utility>

namespace bitui
{
    template<typename Event>
    class Channel;

    template<typename Event>
    class Connection
    {
      public:
        using IdType = uint64_t;

        constexpr Connection() noexcept = default;

        Connection(Connection&& other) noexcept
          : m_channel{ std::exchange(other.m_channel, nullptr) }
          , m_id{ std::exchange(other.m_id, 0) }
        {
        }

        Connection& operator=(Connection&& other) noexcept
        {
            if (this == &other) {
                return *this;
            }

            disconnect();

            m_channel = std::exchange(other.m_channel, nullptr);
            m_id = std::exchange(other.m_id, 0);
            return *this;
        }

        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;

        ~Connection() { disconnect(); }

        void disconnect() noexcept
        {
            if (!m_channel) {
                return;
            }

            auto* Channel{ std::exchange(m_channel, nullptr) };
            auto id{ std::exchange(m_id, 0) };
            Channel->disconnect(id);
        }

        explicit operator bool() const noexcept { return m_channel; }

      private:
        friend class Channel<Event>;

        constexpr Connection(Channel<E>& Channel, IdType id) noexcept
          : m_channel{ &Channel }
          , m_id{ id }
        {
        }

        Channel<Event>* m_channel{};
        IdType m_id{};
    };

    template<typename Event>
    class Channel
    {
      public:
        using event_type = E;

        Channel() = default;

        Channel(const Channel&) = delete;
        Channel& operator=(const Channel&) = delete;

        Channel(Channel&&) = delete;
        Channel& operator=(Channel&&) = delete;

        void reserve(std::size_t count) { slots_.reserve(count); }

        [[nodiscard]] std::size_t size() const noexcept { return slots_.size(); }

        [[nodiscard]] bool empty() const noexcept { return slots_.empty(); }

        void clear() noexcept
        {
            if (emitting_ != 0) {
                for (auto& slot : slots_) {
                    slot.alive = false;
                }
                return;
            }

            slots_.clear();
        }

        template<class F>
            requires(!std::is_const_v<std::remove_reference_t<F>>) && std::invocable<F&, const E&>
        [[nodiscard]] connection listen(F& fn)
        {
            using Fn = std::remove_reference_t<F>;
            return add(std::addressof(fn), &call_lvalue<Fn>);
        }

        template<auto Fn>
            requires std::invocable<decltype(Fn), const E&>
        [[nodiscard]] connection listen()
        {
            return add(nullptr, &call_static<Fn>);
        }

        template<auto MemFn, class T>
            requires std::invocable<decltype(MemFn), T&, const E&>
        [[nodiscard]] connection listen(T& object)
        {
            return add(std::addressof(object), &call_member<MemFn, T>);
        }

        // Returns true if any listener returns true.
        // A true return value stops propagation.
        [[nodiscard]] bool emit(const E& e)
        {
            ++emitting_;

            struct guard
            {
                Channel& self;

                ~guard() noexcept
                {
                    --self.emitting_;
                    if (self.emitting_ == 0) {
                        self.sweep();
                    }
                }
            } guard{ *this };

            const auto count = slots_.size();

            for (std::size_t i = 0; i < count; ++i) {
                if (!slots_[i].alive) {
                    continue;
                }

                auto* object = slots_[i].object;
                auto* call = slots_[i].call;

                if (call(object, e)) {
                    return true;
                }
            }

            return false;
        }

      private:
        using call_fn = bool (*)(void*, const E&);

        struct slot
        {
            connection::id_type id{};
            void* object{};
            call_fn call{};
            bool alive{ true };
        };

        [[nodiscard]] connection add(void* object, call_fn call)
        {
            const auto id = next_id_++;

            slots_.push_back(slot{ .id = id, .object = object, .call = call, .alive = true });

            return connection{ this, id, &disconnect_from_channel };
        }

        static void disconnect_from_channel(void* self, connection::id_type id) noexcept
        {
            static_cast<Channel*>(self)->disconnect(id);
        }

        void disconnect(connection::id_type id) noexcept
        {
            for (auto it = slots_.begin(); it != slots_.end(); ++it) {
                if (it->id != id) {
                    continue;
                }

                if (emitting_ != 0) {
                    it->alive = false;
                }
                else {
                    slots_.erase(it);
                }

                return;
            }
        }

        void sweep() noexcept
        {
            std::erase_if(slots_, [](const slot& s) { return !s.alive; });
        }

        template<class F>
        static bool call_lvalue(void* object, const E& e)
        {
            auto& fn = *static_cast<F*>(object);

            if constexpr (std::same_as<std::invoke_result_t<F&, const E&>, bool>) {
                return std::invoke(fn, e);
            }
            else {
                std::invoke(fn, e);
                return false;
            }
        }

        template<auto Fn>
        static bool call_static(void*, const E& e)
        {
            if constexpr (std::same_as<std::invoke_result_t<decltype(Fn), const E&>, bool>) {
                return std::invoke(Fn, e);
            }
            else {
                std::invoke(Fn, e);
                return false;
            }
        }

        template<auto MemFn, class T>
        static bool call_member(void* object, const E& e)
        {
            auto& instance = *static_cast<T*>(object);

            if constexpr (std::same_as<std::invoke_result_t<decltype(MemFn), T&, const E&>, bool>) {
                return std::invoke(MemFn, instance, e);
            }
            else {
                std::invoke(MemFn, instance, e);
                return false;
            }
        }

        std::vector<slot> slots_;
        connection::id_type next_id_{ 1 };
        std::uint32_t emitting_{};
    };
}