#ifndef __SNL_OBJECT_H_
#define __SNL_OBJECT_H_

#include <string>

class Card;
class CardItem;

namespace SNL {

class SNLObject {
  public:
    ///\return a string describing the object type
    virtual constexpr const char* getTypeName() const = 0;
    ///\return a simple string describing the object. Usually object name.
    virtual std::string getString() const = 0;
    ///\return a string extensively describing the object. Useful for debug.
    virtual std::string getDescription() const = 0;
    virtual Card* getCard() const;

    ///destroy this SNLObject and remove it cleanly from SNL.
    virtual void destroy();
  protected:
    SNLObject() = default;
    virtual ~SNLObject() = default;

    static void preCreate() {}
    void postCreate();
    virtual void preDestroy();
};

}

#endif /* __SNL_OBJECT_H_ */
