type label = string
and classname = string
and fieldname = string
and methodsig = string
and methodname = string;
datatype LABEL =
    StringLabel of string
  | IntLabel of int
datatype JVMBASETYPE = (* fuer ldc, double und long kann dml nicht *)
    JVMInt of int
  | JVMFloat of real
  | JVMString of string
datatype
    INSTRUCTION =
    Astore of int
  | Aastore
  | Aconst of int
  | Aload of int
  | Anewarray of classname
  | Areturn
  | Athrow
  | Bipush of int
  | Catch of classname * label * label * label
  | Checkcast of classname
  | Dup
  | Fconst of int
  | Getfield of classname * fieldname
  | Getstatic of classname * fieldname
  | Goto of label
  | Iconst of int
  | Ifacmp of label
  | Ifeq  of label
  | Ifneq of label
  | Ifnull of label
  | Ireturn of int
  | Instanceof of classname
  | Invokespecial of classname * methodname * methodsig
  | Invokevirtual of classname * methodname * methodsig
  | Label of label
  | Ldc of JVMBASETYPE
  | New of classname
  | Pop
  | Putfield of classname * fieldname
  | Putstatic of classname * fieldname
  | Return
  | Sipush of int
  | Swap
  | Tableswitch of int * (label list) * label
and CLASS =
    Class of CLASSACCESS list * classname * classname * FIELD list * METHOD list
(* klasse, oberklasse *)
and
    FIELD =
    Field of FIELDACCESS list * fieldname * FIELDTYPE
and
    METHOD =
    Method of METHODACCESS list * methodname * methodsig * LIMITS * INSTRUCTION list
and
    FIELDTYPE =
    Classtype of classname
  | Sonstwas of int
and
    CLASSACCESS =
    CPublic | CFinal | CSuper | CAbstract | CInterface
and
    FIELDACCESS =
    FPublic | FPrivate | FProtected | FStatic | FFinal | FVolatile | FTransient
and
    METHODACCESS =
    MPublic | MPrivate | MProtected | MStatic | MFinal | MSynchronized | MNative | MAbstract
and
    LIMITS =
    Limits of int * int
