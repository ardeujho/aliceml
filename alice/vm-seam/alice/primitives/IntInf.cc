//
// Authors:
//   Guido Tack <tack@ps.uni-sb.de>
//
// Copyright:
//   Guido Tack, 2004
//
// Last Change:
//   $Date$ by $Author$
//   $Revision$
//

#include "alice/Authoring.hh"
#include "generic/FinalizationSet.hh"

/*
 * Integers with arbitrary precision 
 *
 * Representation: If the number can be represented as a normal
 * 31-bit Seam int, it is represented that way. Otherwise, a
 * BigInt is allocated, which is a ConcreteRepresentation containing
 * a chunk with the actual gmp integer handle.
 * Operations are performed on the smallest possible type (see TODO below).
 * Return values are normalized.
 *
 */

/*
 * TODO:
 * div, mod, quot, rem, divMod, quotRem, shiftl, shiftr, log2, pow
 * are always performed on BigInts. Special versions for Seam ints
 * should be implemented.
 *
 * The gmp allocation functions are not customized, so gmp ints are
 * allocated on the heap, probably with malloc. They should however
 * be allocated with the same allocation Seam uses (e.g. VirtualAlloc
 * on Windows), maybe one even wants to have a freelist-based memory
 * management.
 *
 * The whole BigInt stuff could be put into seam instead of the Alice
 * language layer. Ideally, a special block tag for big ints could be reserved
 * so that it is easier for the pickler and garbage collector to identify them.
 *
 */

namespace {

  class AliceDll GMPHandler : public ConcreteRepresentationHandler {
  public:
    // Define the format of the external representation
    static const size_t nails = 0; // don't ignore leading stuff
    static const int order = 1; // most significant word first
    static const int endian = 1; // most significant byte first
    static const int size = 4; // words of 4 bytes

    Transform *GetAbstractRepresentation(ConcreteRepresentation *cr) {
      // Pickle a big integer

      BigInt *b = BigInt::FromWordDirect(cr->Get(0));
      MP_INT *value = b->big();    

      int sig = mpz_sgn(value);

      Chunk *c;

      // External representation:
      // Chunk, byte 0 is the sign, the rest is the byte string
      // returned by mpz_export in the format specified above
      if (sig==0) {
	c = Store::AllocChunk(1);
	c->GetBase()[0] = 0; // 0 means 0
      } else {
	// compute required space      
	u_int numb = 8*size - nails;
	u_int count = (mpz_sizeinbase(value, 2) + numb-1) / numb;
	c = Store::AllocChunk(count*size+1);
	char *base = c->GetBase();
	if (sig<0)
	  base[0] = 2; // 2 means negative
	else
	  base[0] = 1; // 1 means positive

	base++;

	size_t counter;
	mpz_export(base, &counter, order, size, endian, nails, value);
      }

      word transformName = String::New("Alice.bigInteger")->ToWord();

      Transform *t = Transform::New(Store::DirectWordToChunk(transformName),
				    c->ToWord());

      return t;
    }
  };

  class AliceDll GMPFinalizationSet: public FinalizationSet {
  public:
    virtual void Finalize(word value);
  };

  void GMPFinalizationSet::Finalize(word value) {
    ConcreteRepresentation *cr = ConcreteRepresentation::FromWordDirect(value);
    word ptr = cr->Get(0);
    BigInt *b = BigInt::FromWordDirect(ptr);
    b->destroy();
  }

}

ConcreteRepresentationHandler *PrimitiveTable::gmpHandler;
FinalizationSet *PrimitiveTable::gmpFinalizationSet;


#define RETURN_INTINF2(i, j)                                    \
{                                                               \
  word res1, res2;                                              \
  s_int ii = i->toInt();                                        \
  if (ii != INVALID_INT) {                                      \
    res1 = Store::IntToWord(ii);                                \
    i->destroy();                                               \
  } else {                                                      \
    ConcreteRepresentation *cr =                                \
    ConcreteRepresentation::New(PrimitiveTable::gmpHandler,1);  \
    cr->Init(0, i->ToWord());                                   \
    res1 = cr->ToWord();                                        \
    PrimitiveTable::gmpFinalizationSet->Register(res1);         \
  }                                                             \
  s_int jj = j->toInt();                                        \
  if (jj != INVALID_INT) {                                      \
    res2 = Store::IntToWord(jj);                                \
    j->destroy();                                               \
  } else {                                                      \
    ConcreteRepresentation *cr =                                \
    ConcreteRepresentation::New(PrimitiveTable::gmpHandler,1);  \
    cr->Init(0, j->ToWord());                                   \
    res2 = cr->ToWord();                                        \
    PrimitiveTable::gmpFinalizationSet->Register(res2);         \
  }                                                             \
  RETURN2(res1, res2);                                          \
}

DEFINE1(IntInf_fromInt) {
  DECLARE_INT(i, x0);
  RETURN_INT(i);
} END

DEFINE1(IntInf_toInt) {
  if (Store::WordToTransient(x0) != INVALID_POINTER) { REQUEST(x0); }
  s_int i = Store::WordToInt(x0);
  if ( i != INVALID_INT)
    { RETURN_INT(i); }
  RAISE(PrimitiveTable::General_Overflow);
} END

static s_int sAbs (const s_int a) {
  return a < 0 ? -a : a;
}

// unary operators
#define MKOP1(op, bigop, smallop)                       \
DEFINE1(IntInf_ ## op) {                                \
  TEST_INTINF(i, x0);                                   \
  if (i==INVALID_INT) {                                 \
    DECLARE_INTINF(i, x0);                              \
    BigInt *res = i->bigop();                           \
    RETURN_INTINF(res);                                 \
  } else {                                              \
    s_int res = smallop(i);                             \
    if (res>=MIN_VALID_INT && res <= MAX_VALID_INT) {   \
      RETURN_INT(res); }                                \
    MK_INTINF(w, BigInt::New(res));                     \
    RETURN(w);                                          \
  }                                                     \
} END
MKOP1(opnegate, negate, -);
MKOP1(abs, abs, sAbs);
MKOP1(notb, notb, ~);
#undef MKOP1

// binary symmetric operators without overflows
#define MKOP2(op, bigop, smallop) \
DEFINE2(IntInf_ ## op) { \
  TEST_INTINF(i, x0);             \
  TEST_INTINF(j, x1);             \
  if (i==INVALID_INT) {           \
    if (j==INVALID_INT) {         \
      DECLARE_INTINF(a, x0);      \
      DECLARE_INTINF(b, x1);      \
      BigInt *res = a->bigop(b);  \
      RETURN_INTINF(res);         \
    } else {                      \
      DECLARE_INTINF(a, x0);      \
      mpz_t jj;                   \
      mpz_init_set_si(jj, j);     \
      BigInt *ret = a->bigop(jj); \
      mpz_clear(jj);              \
      RETURN_INTINF(ret);         \
    }                             \
  } else if (j==INVALID_INT) {    \
      DECLARE_INTINF(b, x1);      \
      mpz_t ii;                   \
      mpz_init_set_si(ii, i);     \
      BigInt *ret = b->bigop(ii); \
      mpz_clear(ii);              \
      RETURN_INTINF(ret);         \
  } else {                        \
    RETURN_INT(i smallop j);      \
  }                               \
} END
MKOP2(orb, orb, |);
MKOP2(xorb, xorb, ^);
MKOP2(andb, andb, &);
#undef MKOP2

// binary operators with integer overflows
#define MKOP2(op, bigop, inversebigop, smallop)         \
DEFINE2(IntInf_ ## op) {                                \
  TEST_INTINF(i, x0);                                   \
  TEST_INTINF(j, x1);                                   \
  if (i==INVALID_INT) {                                 \
    if (j==INVALID_INT) {                               \
      DECLARE_INTINF(a, x0);                            \
      DECLARE_INTINF(b, x1);                            \
      BigInt *res = a->bigop(b);                        \
      RETURN_INTINF(res);                               \
    } else {                                            \
      DECLARE_INTINF(a, x0);                            \
      if (j>0)                                          \
        { BigInt *res = a->bigop(j);                    \
          RETURN_INTINF(res); }                         \
      else                                              \
        { BigInt *res = a->inversebigop(-j);            \
          RETURN_INTINF(res); }                         \
    }                                                   \
  } else if (j==INVALID_INT) {                          \
      DECLARE_INTINF(b, x1);                            \
      if (i>0)                                          \
        { BigInt *res = b->bigop(i);                    \
          RETURN_INTINF(res); }                         \
      else                                              \
        { BigInt *res = b->inversebigop(-i);            \
          RETURN_INTINF(res); }                         \
  } else {                                              \
    s_int res = i smallop j;                            \
    if (res>=MIN_VALID_INT && res <= MAX_VALID_INT) {   \
      RETURN_INT(res); }                                \
    MK_INTINF(w, BigInt::New(res));                     \
    RETURN(w);                                          \
  }                                                     \
} END
MKOP2(opsub, sub, add, -);
MKOP2(opadd, add, sub, +);
#undef MKOP2

// Overflow test, copied from Int.cc
static inline bool CheckProduct(s_int i, s_int j) {
  if (j == 0)
    return false;
  else if (j > 0)
    if (i > 0)
      return i > MAX_VALID_INT / j;
    else // i < 0
      return -i > -MIN_VALID_INT / j;
  else // j < 0
    if (i > 0)
      return i > -MIN_VALID_INT / -j;
    else // i < 0
      return -i > MAX_VALID_INT / -j;
}

DEFINE2(IntInf_opmul) {
  TEST_INTINF(i, x0);
  TEST_INTINF(j, x1);
  if (i==INVALID_INT) {
    if (j==INVALID_INT) {
      DECLARE_INTINF(a, x0);
      DECLARE_INTINF(b, x1);
      RETURN_INTINF(a->mul(b));
    } else {
      DECLARE_INTINF(a, x0);
      BigInt *res = a->mul(j);
      RETURN_INTINF(res);
    }
  } else if (j==INVALID_INT) {
      DECLARE_INTINF(b, x1);
      BigInt *res = b->mul(i);
      RETURN_INTINF(res);
  } else {
    if (CheckProduct(i, j)) {
      BigInt *a = BigInt::New(i);
      BigInt *ret = a->mul(j);
      RETURN_INTINF(ret);
    } else {
      s_int res = i * j;
      RETURN_INT(res);
    }
  }  
} END

DEFINE2(IntInf_pow) {
  TEST_INTINF(base, x0);
  DECLARE_INT(exp, x1);
  if (exp == 0) {
    RETURN_INT(1);
  }
  if (exp < 0) {
    if (base == 0) {
      RAISE(PrimitiveTable::General_Div);
    }
    if (base == 1) {
      RETURN_INT(1);
    }
    if (base == -1) {
      RETURN_INT(-exp % 2 ? -1 : 1);
    }
    RETURN_INT(0);
  }
  DECLARE_INTINF_PROMOTE(a, flag, x0);
  BigInt *res = a->pow(exp);
  DISCARD_PROMOTED(a, flag);
  RETURN_INTINF(res);
} END

// division operators
/*--* should be able to use operations on small ints */
#define MKOPDIV(op, bigop)                      \
DEFINE2(IntInf_ ## op) {                        \
  DECLARE_INTINF_PROMOTE(a, flagA, x0);         \
  DECLARE_INTINF_PROMOTE(b, flagB, x1);         \
  if (*b == 0) {                                \
    DISCARD_PROMOTED(a, flagA);                 \
    DISCARD_PROMOTED(b, flagB);                 \
    RAISE(PrimitiveTable::General_Div);         \
  }                                             \
  BigInt *res = a->bigop(b);                    \
  DISCARD_PROMOTED(a, flagA);                   \
  DISCARD_PROMOTED(b, flagB);                   \
  RETURN_INTINF(res);                           \
} END
MKOPDIV(div, div);
MKOPDIV(mod, mod);
MKOPDIV(quot, quot);
MKOPDIV(rem, rem);
#undef MKOPDIV

// binary boolean operators
#define MKOP2(op, bigop, inversebigop, smallop) \
DEFINE2(IntInf_ ## op) {                        \
  TEST_INTINF(i, x0);                           \
  TEST_INTINF(j, x1);                           \
  if (i==INVALID_INT) {                         \
    DECLARE_INTINF(a, x0);                      \
    if (j==INVALID_INT) {                       \
      DECLARE_INTINF(b, x1);                    \
      RETURN_BOOL(a->bigop(b));                 \
    } else {                                    \
      RETURN_BOOL(a->bigop(j));                 \
    }                                           \
  } else if (j==INVALID_INT) {                  \
      DECLARE_INTINF(b, x1);                    \
      RETURN_BOOL(b->inversebigop(i));          \
  } else {                                      \
    RETURN_BOOL(i smallop j);                   \
  }                                             \
} END
MKOP2(opless, less, greater, <);
MKOP2(opgreater, greater, less, >);
MKOP2(oplessEq, lessEq, greaterEq, <=);
MKOP2(opgreaterEq, greaterEq, lessEq, >=);
#undef MKOP2

DEFINE2(IntInf_compare) {
  TEST_INTINF(i, x0);
  TEST_INTINF(j, x1);
  s_int res;
  if (i==INVALID_INT) {
    DECLARE_INTINF(a, x0);
    if (j==INVALID_INT) {
      DECLARE_INTINF(b, x1);
      res = a->compare(b);
    } else {
      res = a->compare(j);
    }
  } else {
    if (j==INVALID_INT) {
      DECLARE_INTINF(b, x1);
      res = -1*b->compare(i);
    } else {
      if (i==j) { RETURN_INT(Types::EQUAL); }
      else if (i<j) { RETURN_INT(Types::LESS); }
      else { RETURN_INT(Types::GREATER); }
    }
  }
  if(res<0) {
    RETURN_INT(Types::LESS);
  } else if (res==0) {
    RETURN_INT(Types::EQUAL);
  } else { // res>0
    RETURN_INT(Types::GREATER);
  }
} END

DEFINE2(IntInf_divMod) {
  DECLARE_INTINF_PROMOTE(a, flagA, x0);
  DECLARE_INTINF_PROMOTE(b, flagB, x1);

  if (*b==0) {
    DISCARD_PROMOTED(a, flagA);
    DISCARD_PROMOTED(b, flagB);
    RAISE(PrimitiveTable::General_Div);
  }

  BigInt *d = BigInt::New();
  BigInt *m = BigInt::New();

  a->divMod(b, d, m);
  DISCARD_PROMOTED(a, flagA);
  DISCARD_PROMOTED(b, flagB);
  RETURN_INTINF2(d,m);
} END

DEFINE2(IntInf_quotRem) {
  DECLARE_INTINF_PROMOTE(a, flagA, x0);
  DECLARE_INTINF_PROMOTE(b, flagB, x1);

  if (*b==0) {
    DISCARD_PROMOTED(a, flagA);
    DISCARD_PROMOTED(b, flagB);
    RAISE(PrimitiveTable::General_Div);
  }

  BigInt *q = BigInt::New();
  BigInt *r = BigInt::New();

  a->quotRem(b, q, r);
  DISCARD_PROMOTED(a, flagA);
  DISCARD_PROMOTED(b, flagB);
  RETURN_INTINF2(q,r);
} END

DEFINE1(IntInf_log2) {
  DECLARE_INTINF_PROMOTE(i, flag, x0);
  if (i->compare(0L) <= 0) {
    DISCARD_PROMOTED(i, flag);
    RAISE(PrimitiveTable::General_Domain);
  }
  
  u_int l = i->log2();
  DISCARD_PROMOTED(i, flag);

  if (l>MAX_VALID_INT)
    { RAISE(PrimitiveTable::General_Overflow); }

  RETURN_INT(l);
} END

DEFINE2(IntInf_shiftl) {
  DECLARE_INTINF_PROMOTE(a, flagA, x0);
  DECLARE_INT(b, x1);
  BigInt *res = a->shiftl(b);
  DISCARD_PROMOTED(a, flagA);
  RETURN_INTINF(res);
} END

DEFINE2(IntInf_shiftr) {
  DECLARE_INTINF_PROMOTE(a, flagA, x0);
  DECLARE_INT(b, x1);
  BigInt *res = a->shiftr(b);
  DISCARD_PROMOTED(a, flagA);
  RETURN_INTINF(res);
} END

static word BigIntegerHandler(word x) {
  // Unpickling of big integers

  Chunk *c = Chunk::FromWordDirect(x);
  char *base = c->GetBase();

  // the first byte is the sign
  if (base[0]==0) {
    return Store::IntToWord(0);
  } else {
    int sig = base[0];
    base = base+1;
    BigInt *b = BigInt::New();
    u_int count = c->GetSize()-1;
    mpz_import(b->big(), count,
               GMPHandler::order, GMPHandler::size,
               GMPHandler::endian, GMPHandler::nails,
               base);
    // sig==2 means negative
    if (sig==2) {
      mpz_neg(b->big(), b->big());
    }

    s_int ii=b->toInt();
    if (ii != INVALID_INT) {
      b->destroy();
      return Store::IntToWord(ii);
    }

    MK_INTINF(w, b);
    return w;
  }
}

static word bigIntegerTransform;

void PrimitiveTable::RegisterIntInf() {
  PrimitiveTable::gmpFinalizationSet =
    new GMPFinalizationSet();
  PrimitiveTable::gmpHandler = new GMPHandler();

  String *bigInteger = String::New("Alice.bigInteger");
  bigIntegerTransform = bigInteger->ToWord();
  RootSet::Add(bigIntegerTransform);
  Unpickler::RegisterHandler(bigInteger, BigIntegerHandler);

  Register("IntInf.fromInt", IntInf_fromInt, 1);
  Register("IntInf.toInt", IntInf_toInt, 1);
  Register("IntInf.~", IntInf_opnegate, 1);
  Register("IntInf.+", IntInf_opadd, 2);
  Register("IntInf.-", IntInf_opsub, 2);
  Register("IntInf.*", IntInf_opmul, 2);
  Register("IntInf.<", IntInf_opless, 2);
  Register("IntInf.>", IntInf_opgreater, 2);
  Register("IntInf.<=", IntInf_oplessEq, 2);
  Register("IntInf.>=", IntInf_opgreaterEq, 2);
  Register("IntInf.abs", IntInf_abs, 1);
  Register("IntInf.compare", IntInf_compare, 2);
  Register("IntInf.div", IntInf_div, 2);
  Register("IntInf.mod", IntInf_mod, 2);
  Register("IntInf.quot", IntInf_quot, 2);
  Register("IntInf.rem", IntInf_rem, 2);
  Register("IntInf.divMod", IntInf_divMod, 2, 2);
  Register("IntInf.quotRem", IntInf_quotRem, 2, 2);
  Register("IntInf.pow", IntInf_pow, 2);
  Register("IntInf.log2", IntInf_log2, 1);
  Register("IntInf.orb", IntInf_orb, 2);
  Register("IntInf.xorb", IntInf_xorb, 2);
  Register("IntInf.andb", IntInf_andb, 2);
  Register("IntInf.notb", IntInf_notb, 1);
  Register("IntInf.<<", IntInf_shiftl, 2);
  Register("IntInf.~>>", IntInf_shiftr, 2);
}
