#ifndef __PERF_TRACKER_H_
#define __PERF_TRACKER_H_

#include <chrono>
#include <string>

class PerfTracker {
  public: 
    using Time = std::chrono::time_point<std::chrono::high_resolution_clock>;
    PerfTracker(const std::string& name);
    ~PerfTracker();
  private:
    std::string name_;
    Time        start_;
};

class PerfTrackers {
};


#endif
