#ifndef __SNL_FLATTENER_H_
#define __SNL_FLATTENER_H_

namespace naja { namespace SNL {

class SNLDesign;
class SNLFlattenerInstanceTree;

class SNLFlattener {
  public:
    void process(const SNLDesign* top);
  private:
    void processTop(SNLFlattenerInstanceTree* tree, const SNLDesign* top);
};

}} // namespace SNL // namespace naja

#endif // __SNL_FLATTENER_H_