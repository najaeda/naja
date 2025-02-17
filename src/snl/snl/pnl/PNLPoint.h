#ifndef __PNL_POINT_H_
#define __PNL_POINT_H_

namespace naja { namespace PNL {

class PNLPoint {
  public:
    PNLPoint(int x, int y): x_(x), y_(y) {}
    int getX() const { return x_; }
    int getY() const { return y_; }
  private:
    int x_;
    int y_;
};


}} // namespace PNL // namespace naja

#endif // __PNL_POINT_H_