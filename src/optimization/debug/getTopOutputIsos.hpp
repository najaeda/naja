#include <print>

static inline void LCOV_EXCL_DIRECTION(const char *fmt, DNLID term, const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl)
{
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf(fmt,
	 dnl.getDNLTerminalFromID(term).getSnlBitTerm()->getString().c_str());

  printf("Direction %s\n", dnl.getDNLTerminalFromID(term)
	 .getSnlBitTerm()
	 ->getDirection()
	 .getString()
	 .c_str());
  // LCOV_EXCL_STOP
#endif
}

static inline void LCOV_EXCL(const char *fmt, DNLID term, const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl)
{
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf(fmt,
	 dnl.getDNLTerminalFromID(term).getSnlBitTerm()->getString().c_str());
  // LCOV_EXCL_STOP
#endif
}
