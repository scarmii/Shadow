#pragma once

#include "shadow/core/core.h"
#include "shadow/core/log.h"

#include <cstdint>

namespace Shadow
{
    template<typename T>
    using Scope = std::unique_ptr<T>;

    template<typename T, typename ...Args>
    constexpr Scope<T> createScope(Args&& ...args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    class Ref
    {
    public:
        Ref(T* ptr = nullptr)
            : m_ptr(ptr)
        {
            m_counter = ptr ? new uint32_t(1) : nullptr;
        }

        template<typename ...Args>
        Ref(T* ptr, Args&&... args)
            : m_ptr(ptr), m_counter(new uint32_t(1))
        {
            new (ptr) T(std::forward<Args>(args)...);
        }

        template<typename U>
        Ref(const Ref<U>& other)
            : m_ptr(static_cast<T*>(other.get())), m_counter(other.getCounter())
        {
            if (m_ptr)
                (*m_counter)++;
        }

        ~Ref()
        {
            release();
        }

        Ref(const Ref<T>& other)
            : m_ptr(other.m_ptr), m_counter(other.m_counter)
        {
            if (m_ptr)
                (*m_counter)++;
        }

        Ref<T>& operator=(const Ref<T>& other)
        {
            if (this != &other)
            {
                release();

                m_ptr = other.m_ptr;
                m_counter = other.m_counter;

                if (m_ptr)
                    (*m_counter)++;
            }
            return *this;
        }

        Ref<T>& operator=(Ref<T>&& other) noexcept
        {
            release();
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
            m_counter = other.m_counter;
            other.m_counter = nullptr;
            return *this;
        }

        const Ref<T>& operator =(T* ptr)
        {
            release();

            m_ptr = ptr;
            m_counter = new uint32_t(1);

            return *this;
        }

        T* operator->() const { return m_ptr; }
        T& operator*() const { return *m_ptr; }

        bool operator==(T* ptr) const { return m_ptr == ptr; }
        bool operator !() const { return !m_ptr; }
        operator bool() const { return m_ptr != nullptr; }

        template<typename U>
        operator Ref<U>() const
        {
            SH_ASSERT((std::is_base_of<T, U>() || std::is_base_of<U, T>()), "invalid type conversion");
            return Ref<U>(this);
        }

        void release()
        {
            if (m_counter)
            {
                if (--(*m_counter) == 0)
                {
                    delete m_ptr;
                    delete m_counter;
               }
            }
        }

        uint32_t useCount() const { return *m_counter; }
        T* get() const { return m_ptr; }
        uint32_t* getCounter() const { return m_counter; }
    private:
        T* m_ptr;
        uint32_t* m_counter;
    };

    template<typename T, typename ...Args>
    constexpr Ref<T> createRef(Args&& ...args)
    {
        return Ref<T>(new T(args...));
    }

    template<typename T, typename U>
    Ref<T> as(const Ref<U>& ref)
    {
        return static_cast<Ref<T>>(ref);
    }

    template<typename T, const uint32_t maxSize>
    struct Array
    {
        std::array<T, maxSize> array;
        uint32_t size = 0;

        inline uint32_t getMaxSize() const { return array.size(); }

        void setAt(const T& arg, uint32_t index)
        {
            SH_ASSERT((index <= maxSize), "index out of range :<");
            array[index] = arg;
            size++;
        }

        const T* data() const { return array.data(); }
        T* data() { return array.data(); }

        const T& operator[](uint32_t index) const
        {
            SH_ASSERT((index <= maxSize), "index out of range :<");
            return array[index];
        }

        T& operator[](uint32_t index)
        {
            SH_ASSERT((index <= maxSize), "index out of range :<");
            return array[index];
        }
    };
}