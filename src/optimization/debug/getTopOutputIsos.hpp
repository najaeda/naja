#define DEBUG_PRINTS

#include <print>

#define DNL_BIT_TERM dnl.getDNLTerminalFromID(term).getSnlBitTerm()
#define DNL_TERM_STRING DNL_BIT_TERM->getString()
#define DNL_TERM_DIRECTION DNL_BIT_TERM->getDirection().getString()

static inline void LCOV_EXCL_DIRECTION(const char *fmt, DNLID term,
				       const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl)
{
#ifdef DEBUG_PRINTS
  std::print("{} {}\n", fmt, DNL_TERM_STRING);
  std::print("Direction {}\n", DNL_TERM_DIRECTION);
#endif
}

static inline void LCOV_EXCL(const char *fmt, DNLID term,
			     const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl)
{
#ifdef DEBUG_PRINTS
  std::print("{} {}\n", fmt, DNL_TERM_STRING);
#endif
}
