#ifndef __SNL_DB0_H_
#define __SNL_DB0_H_

//SNLDB0 is just another SNLDB with the following characteristics:
//   - it is the first SNLDB of SNLUniverse (ID:0)
//   - it is created automatically and only with Universe creation
//   - it is deleted automatically and only with Universe deletion
//   - it holds special library with special primitives such as assign,....

namespace SNL {

class SNLUniverse;
class SNLDB;

class SNLDB0 {
  friend class SNLUniverse;
  private:
    static SNLDB* create(SNLUniverse* universe);
};

}

#endif /* __SNL_DB0_H_ */
