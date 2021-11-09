#ifndef __SNL_DB0_H_
#define __SNL_DB0_H_

namespace SNL {

class SNLUniverse;
class SNLDB;

/**
 * \brief SNLDB0 is a SNLDB automatically managed by SNLUniverse.
 *
 * SNLDB0 has the following characteristics:
 *   - it is the first SNLDB of SNLUniverse (SNLID::DBID=0)
 *   - it is created automatically and only with SNLUniverse creation
 *   - it is deleted automatically and only with SNLUniverse deletion
 *   - it holds special SNLLibrary with special primitives such as assign,....
 *   - Those special components are accesssible through static methods available in SNLUniverse 
 */
class SNLDB0 {
  friend class SNLUniverse;
  private:
    static SNLDB* create(SNLUniverse* universe);
};

}

#endif /* __SNL_DB0_H_ */
