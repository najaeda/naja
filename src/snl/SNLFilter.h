#ifndef __SNL_FILTER_H_
#define __SNL_FILTER_H_

template<class Type> class SNLFilter {
  public:
    SNLFilter() = default;
    SNLFilter(const SNLFilter&) = default;
    SNLFilter(const SNLFilter&&) = delete;
    virtual ~SNLFilter() {}
};

template<class Type> class SNLNonNULLFilter: public SNLFilter<Type> {
};

#endif /* __SNL_FILTER_H_ */
