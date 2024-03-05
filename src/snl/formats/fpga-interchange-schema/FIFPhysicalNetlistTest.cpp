#include "FIFPhysicalNetlist.h"

int main(int argc, char** argv) {
  naja::FIFPhysicalNetlist::load(argv[1]);
  return 0;
}