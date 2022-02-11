#ifndef __DNL_EQUI_H_
#define __DNL_EQUI_H_

#include <string>

namespace DNL {

class DNLEqui {
  public:
    DNLEqui() = delete;
    DNLEqui(const DNLEqui&) = default;
    DNLEqui(DNLEqui&&) = default;
    DNLEqui(size_t start, size_t stop, bool hasDriver=true):
      start_(start),
      stop_(stop),
      hasDriver_(hasDriver)
    {
      assert(stop_ >= start_);
    }
    size_t getStart() const { return start_; }
    size_t getStop() const { return stop_; }
    bool hasDriver() const { return hasDriver_; }
  private:
    size_t start_;
    size_t stop_;
    bool   hasDriver_  { true };
};

}

#endif /* __DNL_EQUI_H_ */
