//
// Author:
//   Guido Tack <tack@ps.uni-sb.de>
// 
// Copyright:
//   Guido Tack, 2003
// 
// Last change:
//   $Date$ by $Author$
//   $Revision$
// 

#include "alice/Authoring.hh"

#define WORD_PRECISION 32
#define STANDARD_WORD_PRECISION 31
#define STANDARD_WORD_NONBITS (STORE_WORD_WIDTH - STANDARD_WORD_PRECISION)
#define STANDARD_WORD_NONBITS_EXP (static_cast<u_int>(1) << STANDARD_WORD_NONBITS)

#define DECLARE_WORDN(w, x, n)						\
  u_int w = Store::WordToInt(x);					\
  if (static_cast<s_int>(w) == INVALID_INT) { REQUEST(x); } else {}	\
  w &= static_cast<u_int>(-1) >> (STORE_WORD_WIDTH - n);

#define DECLARE_STANDARD_WORD(w, x) \
  DECLARE_WORDN(w, x, STANDARD_WORD_PRECISION)

#define WORD32_WORD_TO_WORD_OP(name, op)        \
  DEFINE2(name) {				\
    DECLARE_WORD32(i, x0);			\
    DECLARE_WORD32(j, x1);			\
    RETURN_WORD32(i op j);			\
  } END

#define WORD32_WORD_TO_BOOL_OP(name, op)	\
  DEFINE2(name) {				\
    DECLARE_WORD32(i, x0);			\
    DECLARE_WORD32(j, x1);			\
    RETURN_BOOL(i op j);			\
  } END

WORD32_WORD_TO_WORD_OP(Word32_opadd, +)
WORD32_WORD_TO_WORD_OP(Word32_opsub, -)
WORD32_WORD_TO_WORD_OP(Word32_opmul, *)

DEFINE1(Word32_opneg) {
  DECLARE_WORD32(i, x0);
  RETURN_WORD32(-i);
} END

WORD32_WORD_TO_BOOL_OP(Word32_opless, <)
WORD32_WORD_TO_BOOL_OP(Word32_opgreater, >)
WORD32_WORD_TO_BOOL_OP(Word32_oplessEq, <=)
WORD32_WORD_TO_BOOL_OP(Word32_opgreaterEq, >=)

// Assume that >> implements a logical right shift (as on x86).

DEFINE2(Word32_opshl) {
  DECLARE_WORD32(i, x0);
  DECLARE_STANDARD_WORD(j, x1);
  if (j >= WORD_PRECISION)
    RETURN_WORD32(0); // see comment in Word.icc
  RETURN_WORD32(i << j);
} END

DEFINE2(Word32_opshr) {
  DECLARE_WORD32(i, x0);
  DECLARE_STANDARD_WORD(j, x1);
  if (j >= WORD_PRECISION)
    RETURN_WORD32(0); // see comment in Word.icc
  RETURN_WORD32(i >> j);
} END

DEFINE2(Word32_oparithshr) {
  DECLARE_WORD32(i, x0);
  DECLARE_STANDARD_WORD(j, x1);
  if (j > WORD_PRECISION - 1) j = WORD_PRECISION - 1; // see above
  //--** this implementation can be improved on many architectures
  if (i & (1 << (WORD_PRECISION - 1))) {
    RETURN_WORD32(i >> j | ~(static_cast<uint32_t>(-1) >> j));
  } else {
    RETURN_WORD32(i >> j);
  }
} END

WORD32_WORD_TO_WORD_OP(Word32_andb, &)

DEFINE2(Word32_div) {
  DECLARE_WORD32(i, x0);
  DECLARE_WORD32(j, x1);
  if (j == 0)
    RAISE(PrimitiveTable::General_Div);
  RETURN_WORD32(i / j);
} END

DEFINE1(Word32_fromInt) {
  DECLARE_INT(i, x0);
  RETURN_WORD32(static_cast<uint32_t>(i));
} END

DEFINE1(Word32_fromLargeWord) {
  RETURN(x0);
} END

DEFINE2(Word32_mod) {
  DECLARE_WORD32(i, x0);
  DECLARE_WORD32(j, x1);
  if (j == 0)
    RAISE(PrimitiveTable::General_Div);
  RETURN_WORD32(i % j);
} END

DEFINE1(Word32_notb) {
  DECLARE_WORD32(i, x0);
  RETURN_WORD32(~i);
} END

WORD32_WORD_TO_WORD_OP(Word32_orb, |)

DEFINE1(Word32_toInt) {
  DECLARE_WORD32(i, x0);
  if (i > static_cast<u_int>(MAX_VALID_INT))
    RAISE(PrimitiveTable::General_Overflow);
  RETURN_INT(i);
} END

DEFINE1(Word32_toIntX) {
  DECLARE_WORD32(i, x0);
  if (static_cast<s_int>(i) > static_cast<s_int>(MAX_VALID_INT) ||
      static_cast<s_int>(i) < static_cast<s_int>(MIN_VALID_INT))
    RAISE(PrimitiveTable::General_Overflow);
  RETURN_INT(i);
} END

DEFINE1(Word32_fromLargeInt) {
  TEST_INTINF(i, x0);
  if (i!=INVALID_INT)
    RETURN_WORD32(static_cast<uint32_t>(i));
  DECLARE_INTINF(ii, x0);
  RETURN_WORD32(static_cast<uint32_t>(mpz_get_ui(ii->big())));
} END

DEFINE1(Word32_toLargeInt) {
  DECLARE_WORD32(i, x0);
  BigInt *b = BigInt::New(static_cast<u_int>(i));
  RETURN_INTINF(b);
} END

DEFINE1(Word32_toLargeIntX) {
  DECLARE_WORD32(i, x0);
  BigInt *b = BigInt::New(static_cast<s_int>(i));
  RETURN_INTINF(b);
} END

DEFINE1(Word32_fromLarge) {
  DECLARE_WORD32(i, x0); // request argument
  RETURN(x0);
} END

DEFINE1(Word32_toLarge) {
  DECLARE_WORD32(i, x0); // request argument
  RETURN(x0);
} END

DEFINE1(Word32_toLargeX) {
  DECLARE_WORD32(i, x0); // request argument
  RETURN(x0);
} END

WORD32_WORD_TO_WORD_OP(Word32_xorb, ^)

void PrimitiveTable::RegisterWord32() {
  Register("Word32.+", Word32_opadd, 2);
  Register("Word32.-", Word32_opsub, 2);
  Register("Word32.*", Word32_opmul, 2);
  Register("Word32.~", Word32_opneg, 1);
  Register("Word32.<", Word32_opless, 2);
  Register("Word32.>", Word32_opgreater, 2);
  Register("Word32.<=", Word32_oplessEq, 2);
  Register("Word32.>=", Word32_opgreaterEq, 2);
  Register("Word32.<<", Word32_opshl, 2);
  Register("Word32.>>", Word32_opshr, 2);
  Register("Word32.~>>", Word32_oparithshr, 2);
  Register("Word32.andb", Word32_andb, 2);
  Register("Word32.div", Word32_div, 2);
  Register("Word32.fromInt", Word32_fromInt, 1);
  Register("Word32.fromLargeInt", Word32_fromLargeInt, 1);
  Register("Word32.fromLarge", Word32_fromLarge, 1);
  Register("Word32.mod", Word32_mod, 2);
  Register("Word32.notb", Word32_notb, 1);
  Register("Word32.orb", Word32_orb, 2);
  Register("Word32.toInt", Word32_toInt, 1);
  Register("Word32.toIntX", Word32_toIntX, 1);
  Register("Word32.toLargeInt", Word32_toLargeInt, 1);
  Register("Word32.toLargeIntX", Word32_toLargeIntX, 1);
  Register("Word32.toLarge", Word32_toLarge, 1);
  Register("Word32.toLargeX", Word32_toLargeX, 1);
  Register("Word32.xorb", Word32_xorb, 2);
}
