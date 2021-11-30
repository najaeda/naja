#include "PerfTracker.h"

PerfTracker::PerfTracker(const std::string& name):
  name_(name),
  start_(std::chrono::high_resolution_clock::now())
{}

PerfTracker::~PerfTracker() {
  //auto stop = std::chrono::high_resolution_clock::now();
}
