#pragma once

#ifdef SH_DEBUG
	#define SH_ASSERT(condition, message, ...) if(!condition)\
	{_log("[ASSERTION FAILED]:", message, TEXT_COLOR_BRIGHT_MAGENTA, ##__VA_ARGS__);\
	 __debugbreak();\
	}
#else
	#define SH_ASSERT(condition, ...)
#endif 

#define SH_CALLBACK(function) std::bind(&function, this, std::placeholders::_1)

#define SH_FLAG(type) bool operator&(type arg1, type arg2);\
type operator|(type arg1, type arg2);\
void operator|=(type& arg1, type arg2);\

#define SH_FLAG_DEF(type, castType) bool operator&(type arg1, type arg2)\
{ return static_cast<castType>(arg1) & static_cast<castType>(arg2); }\
void operator|=(type& arg1, type arg2)\
{ arg1 = arg1 | arg2; }\
type operator|(type arg1, type arg2)\
{ return static_cast<type>(static_cast<castType>(arg1) | static_cast<castType>(arg2)); }\

////////////// Vulkan //////////////////////////////////////////////////////////
#define VK_LOAD_FUNC(instance, func) (PFN_##func)vkGetInstanceProcAddr(instance, #func)
#define VK_CHECK_RESULT(funcCall) {VkResult result = funcCall;\
if(result != VK_SUCCESS)\
{_log("[VULKAN ERROR]:", "%s returned %i :{", TEXT_COLOR_BRIGHT_RED, #funcCall, result);\
__debugbreak();\
}\
}
#ifdef SH_DEBUG
	#define VK_TRACE(message) _log("[VULKAN TRACE]:", message, TEXT_COLOR_CYAN)
	#define VK_WARN(message) _log("[VULKAN WARN]:", message, 	TEXT_COLOR_BRIGHT_YELLOW)
	#define VK_ERROR(message) _log("[VULKAN ERROR]:", message, TEXT_COLOR_BRIGHT_RED)
#elif SH_VULKAN_VALIDATION_LAYERS
	#define VK_TRACE(message) _log("[VULKAN TRACE]:", message, TEXT_COLOR_CYAN)
	#define VK_WARN(message) _log("[VULKAN WARN]:", message, 	TEXT_COLOR_BRIGHT_YELLOW)
	#define VK_ERROR(message) _log("[VULKAN ERROR]:", message, TEXT_COLOR_BRIGHT_RED)
#else
	#define VK_TRACE(message) 
	#define VK_WARN(message) 
	#define VK_ERROR(message) 
#endif 
////////////////////////////////////////////////////////////////////////////////

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
        Ref()
            : m_counter(new int(1))
        {
        }

        Ref(T* ptr)
            : m_ptr(ptr), m_counter(new int(1))
        {
        }

        template<typename U>
        Ref(U* ptr, int* counter)
            : m_ptr(static_cast<T*>(ptr)), m_counter(counter)
        {
            if (m_ptr)
                (*m_counter)++;
        }

        ~Ref()
        {
            if (m_ptr && --(*m_counter) == 0)
            {
                delete m_counter;
                delete m_ptr;
            }
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
                if (m_ptr && --(*m_counter) == 0)
                {
                    delete m_ptr;
                    delete m_counter;
                }

                m_ptr = other.m_ptr;
                m_counter = other.m_counter;

                if (m_ptr)
                    (*m_counter)++;
            }
            return *this;
        }

        Ref<T>& operator=(Ref<T>&& other) noexcept
        {
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
            m_counter = other.m_counter;
            other.m_counter = nullptr;
            return *this;
        }

        const Ref<T>& operator =(T* ptr)
        {
            if (m_ptr && --(*m_counter) == 0)
            {
                delete m_ptr;
                delete m_counter;
            }

            m_ptr = ptr;
            m_counter = new int(1);

            return *this;
        }

        T* operator->() const { return m_ptr; }
        bool operator !() const { return !m_ptr; }
        operator bool() const { return m_ptr != nullptr; }

        template<typename U>
        operator Ref<U>() const
        {
            SH_ASSERT((std::is_base_of<T, U>() || std::is_base_of<U, T>()), "invalid type conversion");
            return Ref<U>(m_ptr, m_counter);
        }

        void release()
        {
            if (m_ptr && --(*m_counter) == 0)
            {
                delete m_ptr;
                delete m_counter;
                return;
            }

            m_ptr = nullptr;
        }

        int getCount() const { return *m_counter; }
        T* get() const { return m_ptr; }
    private:
        T* m_ptr = nullptr;
        int* m_counter;
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