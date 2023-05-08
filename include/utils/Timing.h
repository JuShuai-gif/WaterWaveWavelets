#pragma once
#include <iostream>
#include <stack>
#include <unordered_map>
#include <chrono>
#include "Logger.h"
#include "common.h"

namespace Utils{
	#define START_TIMING(timerName) \
	Utils::Timing::startTiming(timerName);

	#define STOP_TIMING \
	Utils::Timing::stopTiming(false);

	#define STOP_TIMING_PRINT \
	Utils::Timing::stopTiming(true);

	#define STOP_TIMING_AVG \
	{ \
	static int timing_timerId = -1; \
	Utils::Timing::stopTiming(false, timing_timerId); \
	}

	#define STOP_TIMING_AVG_PRINT \
	{ \
	static int timing_timerId = -1; \
	Utils::Timing::stopTiming(true, timing_timerId); \
	}

	#define INIT_TIMING \
		int Utils::IDFactory::id=0; \
		std::unordered_map<int, Utils::AverageTime> Utils::Timing::m_averageTimes; \
		std::stack<Utils::TimingHelper> Utils::Timing::m_timingStack; \
		bool Utils::Timing::m_dontPrintTimes=false; \
		unsigned int Utils::Timing::m_startCounter=0; \
		unsigned int Utils::Timing::m_stopCounter=0;

    struct TimingHelper
    {
        std::chrono::time_point<std::chrono::high_resolution_clock> start;
        std::string name;
    };

    struct  AverageTime
    {
        double totalTime;
        unsigned int counter;
        std::string name;
    };

    class IDFactory
    {
    private:
        static int id;
    public:
        static int getId(){
            return id++;
        }
    };

    class Timing
    {
    public:
        static bool m_dontPrintTimes;
        static unsigned int m_startCounter;
        static unsigned int m_stopCounter;
        static std::stack<TimingHelper> m_timingStack;
        static std::unordered_map<int,AverageTime> m_averageTimes;

        static void reset(){
            while (!m_timingStack.empty())
                m_timingStack.pop();
            m_averageTimes.clear();
            m_startCounter = 0;
            m_stopCounter = 0;
        }

        FORCE_INLINE static void startTiming(const std::string& name=std::string("")){
            TimingHelper h;
            h.start = std::chrono::high_resolution_clock::now();
            h.name = name;
            Timing::m_timingStack.push(h);
            Timing::m_startCounter++;
        }

        FORCE_INLINE static double stopTiming(bool print = true){
            if (!Timing::m_timingStack.empty())
            {
                Timing::m_stopCounter++;
                std::chrono::time_point<std::chrono::high_resolution_clock> stop = std::chrono::high_resolution_clock::now();
                TimingHelper h = Timing::m_timingStack.top();
                Timing::m_timingStack.pop();
                std::chrono::duration<double> elapsed_seconds = stop - h.start;
                double t = elapsed_seconds.count() * 1000.0f;

                if(print)
                {
                    LOG_INFO << "time " << h.name.c_str() << ": " << t << "ms";
                    std::cout << "time " << h.name.c_str() << ": " << t << "ms";
                }
                return t;
            }
            return 0;
        }

        FORCE_INLINE static double stopTiming(bool print,int& id){
            if(id == -1)
                id = IDFactory::getId();
            if(!Timing::m_timingStack.empty()){
                Timing::m_stopCounter++;
                std::chrono::time_point<std::chrono::high_resolution_clock> stop = std::chrono::high_resolution_clock::now();
                TimingHelper h = Timing::m_timingStack.top();
                Timing::m_timingStack.pop();

                std::chrono::duration<double> elapsed_seconds = stop-h.start;
                double t = elapsed_seconds.count() * 1000.0f;

                if(print && !Timing::m_dontPrintTimes)
                    LOG_INFO << "time " << h.name.c_str() << ": " << t << " ms";
                
                if(id >= 0){
                    std::unordered_map<int,AverageTime>::iterator iter;
                    iter = Timing::m_averageTimes.find(id);
                    if(iter != Timing::m_averageTimes.end()){
                        Timing::m_averageTimes[id].totalTime +=t;
                        Timing::m_averageTimes[id].counter++;
                    }
                    else{
                        AverageTime at;
                        at.counter = 1;
                        at.totalTime = t;
                        at.name = h.name;
                        Timing::m_averageTimes[id] = at;
                    }
                }
                return t;
            }
            return 0;
        }

        FORCE_INLINE static void printAverageTimes(){
            std::unordered_map<int,AverageTime>::iterator iter;
            for (iter = Timing::m_averageTimes.begin(); iter!= Timing::m_averageTimes.end();iter++)
            {
                AverageTime &at = iter->second;
                const double avgTime = at.totalTime / at.counter;
                LOG_INFO << "Average time " << at.name.c_str() << ": " << avgTime << " ms";
            }
            if(Timing::m_startCounter != Timing::m_stopCounter)
                LOG_INFO << "Problem: " << Timing::m_startCounter << " calls of startTiming and " << Timing::m_stopCounter << " calls of stopTiming. ";
            LOG_INFO << "--------------------------------------------\n";
        }

        FORCE_INLINE static void printTimeSums(){
            std::unordered_map<int,AverageTime>::iterator iter;
            for (iter = Timing::m_averageTimes.begin();iter!=Timing::m_averageTimes.end();iter++)
            {
                AverageTime &at = iter->second;
                const double timeSum = at.totalTime;
                LOG_INFO << "Time sum " << at.name.c_str() << ": " << timeSum << " ms";
            }
            if(Timing::m_startCounter != Timing::m_stopCounter)
                LOG_INFO << "Problem: " << Timing::m_startCounter << " calls of startTiming and " << Timing::m_stopCounter << " call of stopTiming. ";
            LOG_INFO << "----------------------------------------------------\n";
        }
    
    };
}