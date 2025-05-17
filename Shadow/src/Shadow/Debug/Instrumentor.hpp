#pragma once

#include "Shadow/Core/ShEngine.hpp"

#include <string>
#include <chrono>
#include <algorithm>
#include <fstream>

#include <thread>

namespace Shadow
{
    struct ProfileResult
    {
        std::string name;
        long long start, end;
        uint32_t threadID;
    };
    
    struct InstrumentationSession
    {
        std::string name;
    };
    
    class Instrumentor
    {
    public:
        static Instrumentor& get()
        {
            static Instrumentor* instance = new Instrumentor();
            return *instance;
        }
    
        void beginSession(const std::string& name, const std::string& filepath = "results.json")
        {
            m_outputStream.open(filepath);
            writeHeader();
            m_currentSession = new InstrumentationSession{ name };
        }
    
        void writeProfile(const ProfileResult& result)
        {
            // useless currently
            std::lock_guard<std::mutex> lock(m_profileWriteMutex);

            if (m_profileCount++ > 0)
                m_outputStream << ",";
            else
                m_outputStream << '\t';
    
            std::string name = result.name;
            std::replace(name.begin(), name.end(), '"', '\"');
    
            m_outputStream << "{\n\t\t";
            m_outputStream << "\"cat\":\"function\",\n\t\t";
            m_outputStream << "\"dur\":" << (result.end - result.start) << ",\n\t\t";
            m_outputStream << "\"name\":\"" << name << "\",\n\t\t";
            m_outputStream << "\"ph\": \"X\",\n\t\t";
            m_outputStream << "\"pid\":0,\n\t\t";
            m_outputStream << "\"tid\":" << result.threadID << ",\n\t\t";
            m_outputStream << "\"ts\":" << result.start << "\n\t";
            m_outputStream << "}";
    
            m_outputStream.flush();
        }
    
    
        void endSession()
        {
            writeFooter();
            m_outputStream.close();
            delete m_currentSession;
            m_currentSession = nullptr;
            m_profileCount = 0;
        }
    private:
        Instrumentor()
            : m_currentSession(nullptr), m_profileCount(0)
        {
        }
        void writeHeader()
        {
            m_outputStream << "{\"otherData\": {}, \"traceEvents\": [\n";
            m_outputStream.flush();
        }
    
        void writeFooter()
        {
            m_outputStream << "\n]}";
            m_outputStream.flush();
        }
    private:
        InstrumentationSession* m_currentSession;
        std::ofstream m_outputStream;
        uint32_t m_profileCount;
        std::mutex m_profileWriteMutex;
    };
    
    class InstrumentationTimer
    {
    public:
        InstrumentationTimer(const char* name)
            : m_name(name), m_stopped(false)
        {
            m_startTimepoint = std::chrono::high_resolution_clock::now();
        }
    
        void stop()
        {
            auto endTimepoint = std::chrono::high_resolution_clock::now();
    
            long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_startTimepoint).time_since_epoch().count();
            long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();
    
            uint32_t threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());
            Instrumentor::get().writeProfile({ m_name, start, end, threadID });
    
            m_stopped = true;
        }
    
        ~InstrumentationTimer()
        {
            if (!m_stopped)
                stop();
        }
    private:
        const char* m_name;
        std::chrono::time_point<std::chrono::steady_clock> m_startTimepoint;
        bool m_stopped;
    };
}

#define SH_PROFILE 1
#if  SH_PROFILE 1
    #define SH_PROFILE_BEGIN_SESSION(name, filepath) Shadow::Instrumentor::get().beginSession(name, filepath);
    #define SH_PROFILE_END_SESSION() Shadow::Instrumentor::get().endSession();
    #define COMBINE(x, y) x ## y
    #define COMB(x, y) COMBINE(x, y)
    #define SH_PROFILE_SCOPE(name) Shadow::InstrumentationTimer COMB(timer, __LINE__)(name);
    #define SH_PROFILE_FUNCTION() SH_PROFILE_SCOPE(__FUNCSIG__)
#else
    #define SH_PROFILE_BEGIN_SESSION(name)
    #define SH_PROFILE_END_SESSION()
    #define SH_PROFILE_SCOPE(name)
    #define SH_PROFILE_FUNCTION()
#endif 

#ifdef SH_PROFILE_RENDERER_FUNCTION
    #define SH_PROFILE_RENDERER_FUNCTION() SH_PROFILE_FUNCTION()
#else
    #define SH_PROFILE_RENDERER_FUNCTION()
#endif

