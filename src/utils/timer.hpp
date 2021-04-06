#pragma once
#include <iostream>
#include <chrono>

namespace cryptobot {

class Timer
{
public:
    void start()
    {
        m_StartTime = std::chrono::steady_clock::now();
        m_checkpoint = std::chrono::steady_clock::now();
        m_bRunning = true;
    }
    
    void stop()
    {
        m_EndTime = std::chrono::steady_clock::now();
        m_bRunning = false;
    }
    
    double checkpoint() {
        if (!m_bRunning) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(m_EndTime - m_checkpoint).count();
        }
        auto now = std::chrono::steady_clock::now();
        double since_last_checkpoint = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_checkpoint).count();
        m_checkpoint = now;
        return since_last_checkpoint;
    }
    double elapsedMilliseconds()
    {
        std::chrono::time_point<std::chrono::steady_clock> endTime;
        
        if(m_bRunning)
        {
            endTime = std::chrono::steady_clock::now();
        }
        else
        {
            endTime = m_EndTime;
        }
        
        return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - m_StartTime).count();
    }
    
    double elapsedSeconds()
    {
        return elapsedMilliseconds() / 1000.0;
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
    std::chrono::time_point<std::chrono::steady_clock> m_EndTime;
    std::chrono::time_point<std::chrono::steady_clock> m_checkpoint;
    bool                                               m_bRunning = false;
};

}