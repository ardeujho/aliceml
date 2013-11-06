//
// Authors:
//   Thorsten Brunklaus <brunklaus@ps.uni-sb.de>
//   Leif Kornstaedt <kornstae@ps.uni-sb.de>
//
// Copyright:
//   Thorsten Brunklaus, 2000
//   Leif Kornstaedt, 2000-2002
//
// Last Change:
//   $Date$ by $Author$
//   $Revision$
//

#include <cmath>
#include <cstdio>
#include "alice/Authoring.hh"

#define REAL_TO_REAL(name, op)			\
  DEFINE1(name) {				\
    DECLARE_REAL(real, x0);			\
    RETURN_REAL(op(real->GetValue()));		\
  } END

#define REAL_REAL_TO_REAL(name, op)			   \
  DEFINE2(name) {					   \
    DECLARE_REAL(real1, x0);				   \
    DECLARE_REAL(real2, x1);				   \
    RETURN_REAL(op(real1->GetValue(), real2->GetValue())); \
  } END

#define REAL_TO_INT(name, op)				\
  DEFINE1(name) {					\
    DECLARE_REAL(real, x0);				\
    double value = real->GetValue();			\
    if (std::isnan(value))				\
      RAISE(PrimitiveTable::General_Domain);      	\
    double result = op(value);				\
    if (result > static_cast<double>(MAX_VALID_INT) ||	\
	result < static_cast<double>(MIN_VALID_INT)) {	\
      RAISE(PrimitiveTable::General_Overflow);      	\
    }							\
    RETURN_INT(static_cast<s_int>(result));		\
  } END

#define REAL_TO_INTINF(name, op)		\
  DEFINE1(name) {				\
    DECLARE_REAL(real, x0);			\
    double value = real->GetValue();		\
    if (std::isnan(value))				\
      RAISE(PrimitiveTable::General_Domain);	\
    double result = op(value);			\
    if (std::isinf(result)) {			\
      RAISE(PrimitiveTable::General_Overflow);	\
    }						\
    BigInt *b = BigInt::New(result);		\
    RETURN_INTINF(b);				\
  } END

#define REAL_REAL_TO_REAL_OP(name, op)				\
  DEFINE2(name) {						\
    DECLARE_REAL(real1, x0);					\
    DECLARE_REAL(real2, x1);					\
    RETURN_REAL(real1->GetValue() op real2->GetValue());	\
  } END

#define REAL_REAL_TO_BOOL_OP(name, op)				\
  DEFINE2(name) {						\
    DECLARE_REAL(real1, x0);					\
    DECLARE_REAL(real2, x1);					\
    RETURN_BOOL(real1->GetValue() op real2->GetValue());	\
  } END

#define REAL_REAL_TO_INT(name, op)				\
  DEFINE2(name) {						\
    DECLARE_REAL(real1, x0);					\
    DECLARE_REAL(real2, x1);					\
    double result = op(real1->GetValue(), real2->GetValue());	\
    RETURN_INT(static_cast<s_int>(result));			\
  } END

static inline double Trunc(double x) {
  if (x >= 0.0)
    return std::floor(x);
  else
    return std::ceil(x);
}

REAL_TO_REAL(Real_opnegate, -)
REAL_REAL_TO_REAL_OP(Real_opadd, +)
REAL_REAL_TO_REAL_OP(Real_opsub, -)
REAL_REAL_TO_REAL_OP(Real_opmul, *)
REAL_REAL_TO_REAL_OP(Real_opdiv, /)
REAL_REAL_TO_BOOL_OP(Real_opless, <)
REAL_REAL_TO_BOOL_OP(Real_opgreater, >)
REAL_REAL_TO_BOOL_OP(Real_oplessEq, <=)
REAL_REAL_TO_BOOL_OP(Real_opgreaterEq, >=)
REAL_TO_INT(Real_ceil, std::ceil)
REAL_TO_INTINF(Real_largeCeil, std::ceil)

DEFINE2(Real_compare) {
  DECLARE_REAL(real1, x0);
  DECLARE_REAL(real2, x1);
  double x = real1->GetValue();
  double y = real2->GetValue();
  if (x == y) {
    RETURN_INT(Types::EQUAL);
  } else if (x < y) {
    RETURN_INT(Types::LESS);
  } else if (x > y) {
    RETURN_INT(Types::GREATER);
  } else {
    RAISE(PrimitiveTable::General_Unordered);
  }
} END

REAL_TO_INT(Real_floor, std::floor)
  REAL_TO_INTINF(Real_largeFloor, std::floor)

DEFINE1(Real_fromInt) {
  DECLARE_INT(i, x0);
  RETURN_REAL(static_cast<double>(i));
} END

DEFINE1(Real_fromLargeInt) {
  DECLARE_INTINF_PROMOTE(i, flag, x0);
  double res = mpz_get_d(i->big()); 
  DISCARD_PROMOTED(i, flag);
  RETURN_REAL(res);
} END

REAL_TO_REAL(Real_realCeil, std::ceil)
  REAL_TO_REAL(Real_realFloor, std::floor)

static inline double Rint(double x) {
  double fx = floor(x);
  double diff = x - fx;
  if (diff > 0.5)
    fx += 1.0;
  else if (diff == 0.5) {
    double f2 = fx / 2.0;
    if (f2 != floor(f2))
      fx += 1.0;
  }
  return fx;
}

REAL_TO_REAL(Real_realRound, Rint)
REAL_TO_REAL(Real_realTrunc, Trunc)
  REAL_REAL_TO_REAL(Real_rem, std::fmod)
REAL_TO_INT(Real_round, Rint)
REAL_TO_INTINF(Real_largeRound, Rint)

DEFINE1(Real_toString) {
  static char buf[50];
  DECLARE_REAL(real, x0);
  double value = real->GetValue();
  if (std::isnan(value)) {
    std::strcpy(buf, "nan");
  } else if (std::isinf(value)) {
    if (value > 0.0)
      std::strcpy(buf, "inf");
    else
      std::strcpy(buf, "~inf");
  } else {
    std::sprintf(buf, "%.12G", value);
    bool hasDecimalPoint = false, done = false;
    u_int i = 0;
    while (!done)
      switch (buf[i++]) {
      case '\0':
	done = true;
	break;
      case '-':
	buf[i - 1] = '~';
	break;
      case ',':
	buf[i - 1] = '.';
	hasDecimalPoint = true;
	break;
      case '.':
	hasDecimalPoint = true;
      }
    if (!hasDecimalPoint) std::strcpy(&buf[i - 1], ".0");
  }
  RETURN(String::New(buf)->ToWord());
} END

REAL_TO_INT(Real_trunc, Trunc)
REAL_TO_INTINF(Real_largeTrunc, Trunc)

void PrimitiveTable::RegisterReal() {
  Register("Real.~", Real_opnegate, 1);
  Register("Real.+", Real_opadd, 2);
  Register("Real.-", Real_opsub, 2);
  Register("Real.*", Real_opmul, 2);
  Register("Real./", Real_opdiv, 2);
  Register("Real.<", Real_opless, 2);
  Register("Real.>", Real_opgreater, 2);
  Register("Real.<=", Real_oplessEq, 2);
  Register("Real.>=", Real_opgreaterEq, 2);
  Register("Real.ceil", Real_ceil, 1);
  Register("Real.largeCeil", Real_largeCeil, 1);
  Register("Real.compare", Real_compare, 2);
  Register("Real.floor", Real_floor, 1);
  Register("Real.largeFloor", Real_largeFloor, 1);
  Register("Real.fromInt", Real_fromInt, 1);
  Register("Real.fromLargeInt", Real_fromLargeInt, 1);
  Register("Real.precision", Store::IntToWord(52));
  Register("Real.realCeil", Real_realCeil, 1);
  Register("Real.realFloor", Real_realFloor, 1);
  Register("Real.realRound", Real_realRound, 1);
  Register("Real.realTrunc", Real_realTrunc, 1);
  Register("Real.rem", Real_rem, 2);
  Register("Real.round", Real_round, 1);
  Register("Real.largeRound", Real_largeRound, 1);
  Register("Real.toString", Real_toString, 1);
  Register("Real.trunc", Real_trunc, 1);
  Register("Real.largeTrunc", Real_largeTrunc, 1);
}
