#ifndef __SNL_EXCEPTION_H_
#define __SNL_EXCEPTION_H_

namespace SNL {

struct SNLException: public std::exception {
  public:
    SNLException() = delete;
    SNLException(const SNLException&) = default;

    SNLException(const std::string& reason):
      std::exception(),
      reason_(reason)
    {}

    std::string getReason() const {
      return reason_;
    }

    const char* what() const noexcept override {
      return reason_.c_str();
    }

  private:
    const std::string reason_;
};

}

#endif /* __SNL_EXCEPTION_H_ */
