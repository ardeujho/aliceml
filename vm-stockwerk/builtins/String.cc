//
// Authors:
//   Thorsten Brunklaus <brunklaus@ps.uni-sb.de>
//   Leif Kornstaedt <kornstae@ps.uni-sb.de>
//
// Copyright:
//   Thorsten Brunklaus, 2000
//   Leif Kornstaedt, 2000
//
// Last Change:
//   $Date$ by $Author$
//   $Revision$
//

#include <cstring>

#include "builtins/Authoring.hh"

DEFINE2(String_opconcat) {
  DECLARE_STRING(string1, x0);
  DECLARE_STRING(string2, x1);
  int length1 = string1->GetLength();
  int length2 = string2->GetLength();
  String *newString = String::New(length1 + length2);
  char *base = newString->GetValue();
  std::memcpy(base, string1->GetValue(), length1);
  std::memcpy(base + length1, string2->GetValue(), length2);
  RETURN(newString->ToWord());
} END

static inline int DoCompare(String *string1, String *string2) {
  int length1 = string1->GetLength();
  int length2 = string2->GetLength();
  int result = std::memcmp(string1->GetValue(), string2->GetValue(),
			   length1 < length2? length1: length2);
  if (result == 0) {
    if (length1 < length2)
      return -1;
    else if (length1 == length2)
      return 0;
    else
      return 1;
  } else
    return result;
}

#define COMPARISON(name, op)				\
  DEFINE2(name) {					\
    DECLARE_STRING(string1, x0);			\
    DECLARE_STRING(string2, x1);			\
    RETURN_BOOL(DoCompare(string1, string2) op 0);	\
  } END

COMPARISON(String_opless, <)
COMPARISON(String_opgreater, >)
COMPARISON(String_oplessEq, <=)
COMPARISON(String_opgreaterEq, >=)

DEFINE2(String_compare) {
  DECLARE_STRING(string1, x0);
  DECLARE_STRING(string2, x1);
  int result = DoCompare(string1, string2);
  if (result < 0) {
    RETURN_INT(2);   // LESS
  } else if (result == 0) {
    RETURN_INT(0);   // EQUAL
  } else { // result > 0
    RETURN_INT(1);   // GREATER
  }
} END

DEFINE1(String_explode) {
  DECLARE_STRING(string, x0);
  char *base = string->GetValue();
  word list = Store::IntToWord(1); // nil
  for (int i = string->GetLength(); i--; ) {
    TagVal *cons = TagVal::New(0, 2);
    cons->Init(0, Store::IntToWord(base[i]));
    cons->Init(1, list);
    list = cons->ToWord();
  }
  RETURN(list);
} END

DEFINE1(String_implode) {
  DECLARE_LIST_ELEMS(tagVal, length, x0, DECLARE_INT(c, tagVal->Sel(0)));
  String *string = String::New(length);
  char *base = string->GetValue();
  int i = 1;
  while (tagVal != INVALID_POINTER) {
    base[i++] = Store::WordToInt(tagVal->Sel(0));
    tagVal = TagVal::FromWord(tagVal->Sel(1));
  }
  RETURN(string->ToWord());
} END

DEFINE1(String_size) {
  DECLARE_STRING(string, x0);
  RETURN_INT(string->GetLength());
} END

DEFINE2(String_sub) {
  DECLARE_STRING(string, x0);
  DECLARE_INT(index, x1);
  if (index < 0 || index >= string->GetLength())
    RAISE(GlobalPrimitives::General_Subscript);
  RETURN_INT(string->GetValue()[index]);
} END

DEFINE3(String_substring) {
  DECLARE_STRING(string, x0);
  DECLARE_INT(startIndex, x1);
  DECLARE_INT(sliceLength, x2);
  int stringLength = string->GetLength();
  if (startIndex < 0 || sliceLength < 0 ||
      startIndex + sliceLength > stringLength)
    RAISE(GlobalPrimitives::General_Subscript);
  String *substring = String::New(sliceLength);
  std::memcpy(substring->GetValue(),
	      string->GetValue() + startIndex, sliceLength);
  RETURN(substring->ToWord());
} END

DEFINE1(String_str) {
  DECLARE_INT(i, x0);
  char c = i;
  RETURN(String::New(&c, 1)->ToWord());
} END

void Primitive::RegisterString() {
  Register("String.^", String_opconcat, 2);
  Register("String.<", String_opless, 2);
  Register("String.>", String_opgreater, 2);
  Register("String.<=", String_oplessEq, 2);
  Register("String.>=", String_opgreaterEq, 2);
  Register("String.compare", String_compare, 2);
  Register("String.explode", String_explode, 1);
  Register("String.implode", String_implode, 1);
  Register("String.maxSize", Store::IntToWord(0x3FFFFFFF));
  Register("String.size", String_size, 1);
  Register("String.sub", String_sub, 1);
  Register("String.substring", String_substring, 3);
  Register("String.str", String_str, 1);
}
