#ifndef __NAJA_OBJECT_H_
#define __NAJA_OBJECT_H_

#include <map>
#include <string>

namespace naja { namespace core {

class Property;

class NajaObject {
  public:
    using Properties = std::map<std::string, Property*>;
    virtual ~NajaObject() = default;

    ///\return a string describing the object type
    virtual constexpr const char* getTypeName() const = 0;
    ///\return a simple string describing the object. Usually object name.
    virtual std::string getString() const = 0;
    ///\return a string extensively describing the object. Useful for debug.
    virtual std::string getDescription() const = 0;
  protected:
    NajaObject() = default;

  private:
    Properties  properties_;
};

}} // namespace core // namespace naja

#endif // __NAJA_OBJECT_H_