structure CheckIntermediate :> CHECK_INTERMEDIATE =
  struct
(*
    structure Env = MakeHashScopedImpMap(FromEqHashKey(Stamp))

    open IntermediateGrammar
    open PervasiveType


  (* Signalling errors *)

    fun fail(r,s) =
	raise Crash.Crash("intermediate inconsistency @ " ^
			  Source.regionToString r ^ ": " ^ s)

    fun regionOfNonInfo {region}     = region	(* I want poly records :-( *)
    fun regionOfTypInfo {region,typ} = region

    fun check true  (i,s)  = ()
      | check false (i,s)  = fail(regionOfTypInfo i, s)

    fun check' f x (i,s)   = f x handle _ => fail(regionOfTypInfo i, s)

    fun checkI true  (i,s) = ()
      | checkI false (i,s) = fail(regionOfNonInfo i, s)

    fun checkI' f x (i,s)  = f x handle _ => fail(regionOfNonInfo i, s)


  (* Helpers *)

    fun resultCon t =
	if      Type.isArrow' t then resultCon(#2(Type.asArrow' t))
	else if Type.isApply' t then resultCon'(#1(Type.asApply' t))
	else if Type.isCon' t   then SOME(Type.asCon' t)
	else NONE
    and resultCon' t =
	if      Type.isApply' t then resultCon'(#1(Type.asApply' t))
	else if Type.isCon' t   then SOME(Type.asCon' t)
	else NONE

    fun hasResultType(t,c) =
	case resultCon t
	  of NONE    => false
	   | SOME c' => c = c'

    fun isOpenType t =
	case resultCon t
	  of NONE   => false
	   | SOME c => #2 c = Type.OPEN

    fun asArrows  t = asArrows'([],t)
    and asArrows'(ts,t) =
	if Type.isArrow' t then
	    let
		val (t1,t2) = Type.asArrow' t
	    in
		asArrows'(t1::ts, t2)
	    end
	else
	    (List.rev ts, t)

    fun unquantify t =
	if      Type.isAll' t   then unquantify(#2(Type.asAll' t))
	else if Type.isExist' t then unquantify(#2(Type.asExist' t))
	else t


  (* Type matching in UpExp *)

    fun matches(t1,t2) = true (*UNFINISHED*)


  (* Literals *)

    fun chLit(WordLit _)   = PervasiveType.typ_word
      | chLit(IntLit _)    = PervasiveType.typ_int
      | chLit(CharLit _)   = PervasiveType.typ_char
      | chLit(StringLit _) = PervasiveType.typ_string
      | chLit(RealLit _)   = PervasiveType.typ_real


  (* Identifiers *)

    fun chLab E (Lab(i, a)) = a

    fun chId E (Id(i, z, n)) =
	let
	    val t = check' Env.lookupExistent(E,z) (i,"Id unknown")
	in
	    check (Type.equals(t, #typ i)) (i,"Id type");
	    t
	end

    fun chBindId E (Id(i, z, n)) =
	let
	    val t = #typ i
	in
	    check' Env.insertDisjoint(E,z,t) (i,"Id bound twice");
	    t
	end

    fun chLongid E =
	fn ShortId(i, id) =>
	   let
		val t = chId E id
	   in
		check (Type.equals(t,#typ i)) (i,"ShortId type");
		t
	   end

         | LongId(i, longid, lab) =>
	   let
		val a  = chLab E lab
		val t1 = chLongid E longid
		val r  = check' Type.asProd' t1       (i,"LongId sel type")
		val ts = check' Type.lookupRow(r,a)   (i,"LongId label")
		val _  = check (Vector.length ts = 1) (i,"LongId field type")
		val t  = Vector.sub(ts,0)
	   in
		check (Type.equals(t,#typ i)) (i,"LongId type");
		t
	   end


  (* Fields *)

    fun chFields chX E fields =
	    List.foldl (chField chX E) (Type.emptyRow()) fields

    and chField chX E (Field(i, lab, x), r) =
	let
	    val a = chLab E lab
	    val t = chX E x
	in
	    checkI' Type.extendRow(a,#[t],r) (i,"Field duplicate")
	end


  (* Expressions *)

    fun chExps E exps = List.map (chExp E) exps
    and chExp E =
	fn LitExp(i, lit) =>
	   let
		val t = chLit lit
	   in
		check (Type.equals(t, #typ i)) (i,"LitExp type");
		t
	   end

	 | PrimExp(i, string) =>
		#typ i

	 | NewExp(i, isNary) =>
	   let
		val t = #typ i
	   in
		check (isOpenType(unquantify t)) (i,"NewExp type sort");
		t
	   end

	 | VarExp(i, longid) =>
	   let
		val t1 = chLongid E longid
		val t  = #typ i
	   in
		check (Type.matches(t1,t)) (i,"VarExp type");
		t
	   end

	 | TagExp(i, lab, isNary) =>
	   let
		val  a       = chLab E lab
		val  t       = #typ i
		val (ts2,t2) = asArrows(unquantify t)
		val  r       = check' Type.asSum' t2      (i,"TagExp type")
		val  ts1     = check' Type.lookupRow(r,a) (i,"TagExp label")
		(*UNFINISHED: check isNary *)
	   in
		check (Vector.length ts1 = List.length ts2) (i,"TagExp arity");
		check (VectorPair.all Type.equals (ts1, Vector.fromList ts2))
						(i,"TagExp argument type");
		t
	   end

	 | ConExp(i, longid, isNary) =>
	   let
		val t1 = chLongid E longid
		val t  = #typ i
		(*UNFINISHED: check isNary *)
	   in
		check (Type.matches(t1,t))       (i,"ConExp type");
		check (isOpenType(unquantify t)) (i,"ConExp type constructor");
		t
	   end

	 | RefExp(i) =>
	   let
		val t  = #typ i
		val t1 = check' (#2 o Type.asArrow' o unquantify) t
						(i,"RefExp type")
		val c  = check' Type.asCon' t1  (i,"RefExp type")
	   in
		check (c = con_ref) (i,"RefExp type constructor");
		t
	   end

	 | TupExp(i, exps) =>
	   let
		val t = Type.inTuple(Vector.fromList(chExps E exps))
	   in
		check (Type.equals(t, #typ i)) (i,"TupExp type");
		t
	   end

	 | ProdExp(i, fields) =>
	   let
		val t = Type.inProd(chFields chExp E fields)
	   in
		check (Type.equals(t, #typ i)) (i,"ProdExp type");
		t
	   end

	 | SelExp(i, lab) =>
	   let
		val  a      = chLab E lab
		val  t      = #typ i
		val (t1,t2) = check' Type.asArrow' t (i,"SelExp type")
		val  r      = check' Type.asProd' t1 (i,"SelExp type")
		val  ts     = check' Type.lookupRow(r,a) (i,"SelExp label")
		val  _      = check (Vector.length ts=1) (i,"SelExp field type")
		val  t1'    = Vector.sub(ts,0)
	   in
		check (Type.equals(t1',t1)) (i,"SelExp field type");
		t
	   end

	 | VecExp(i, exps) =>
	   let
		val  ts     = chExps E exps
		val  t      = #typ i
		val (t1,t2) = check' Type.asApply' t (i,"VecExp type")
		val  c      = check' Type.asCon' t1  (i,"VecExp type")
	   in
		check (c = con_vec) (i,"VecExp type constructor");
		check (List.all (fn t' => Type.equals(t',t2)) ts)
				    (i,"VecExp component type");
		t
	   end

	 | FunExp(i, matchs) =>
	   let
		val  t      = #typ i
		val (t1,t2) = check' Type.asArrow' t (i,"FunExp type")
		val  tts    = chMatchs E matchs
	   in
		check (List.all (fn(t3,t4) =>
			Type.equals(t3,t1) andalso Type.equals(t4,t2)) tts)
		      (i,"FunExp match types");
		t
	   end

	 | AppExp(i, exp1, exp2) =>
	   let
		val  t        = #typ i
		val  t1       = chExp E exp1
		val  t2       = chExp E exp2
		val (t11,t12) = check' Type.asArrow' t1 (i,"AppExp func type")
	   in
		check (Type.equals(t11,t2)) (i,"AppExp argument type");
		check (Type.equals(t12, t)) (i,"AppExp result type");
		t
	   end

	 | AdjExp(i, exp1, exp2) =>
	   let
		val t  = #typ i
		val t1 = chExp E exp1
		val t2 = chExp E exp2
	   in
		check (Type.isProd' t orelse Type.isTuple' t)
					   (i,"AdjExp type");
		check (Type.equals(t1, t)) (i,"AdjExp left operand type");
		check (Type.equals(t2, t)) (i,"AdjExp right operand type");
		t
	   end

	 | AndExp(i, exp1, exp2) =>
	   let
		val t  = #typ i
		val t1 = chExp E exp1
		val t2 = chExp E exp2
	   in
		check (Type.equals(t, typ_bool)) (i,"AndExp type");
		check (Type.equals(t1,typ_bool)) (i,"AndExp left operand type");
		check (Type.equals(t2,typ_bool)) (i,"AndExp right operand type");
		t
	   end

	 | OrExp(i, exp1, exp2) =>
	   let
		val t  = #typ i
		val t1 = chExp E exp1
		val t2 = chExp E exp2
	   in
		check (Type.equals(t, typ_bool)) (i,"OrExp type");
		check (Type.equals(t1,typ_bool)) (i,"OrExp left operand type");
		check (Type.equals(t2,typ_bool)) (i,"OrExp right operand type");
		t
	   end

	 | IfExp(i, exp1, exp2, exp3) =>
	   let
		val t  = #typ i
		val t1 = chExp E exp1
		val t2 = chExp E exp2
		val t3 = chExp E exp3
	   in
		check (Type.equals(t1, typ_bool)) (i,"IfExp condition type");
		check (Type.equals(t2, t))        (i,"IfExp then branch type");
		check (Type.equals(t3, t))        (i,"IfExp else branch type");
		t
	   end

	 | WhileExp(i, exp1, exp2) =>
	   let
		val t  = #typ i
		val t1 = chExp E exp1
		val t2 = chExp E exp2
	   in
		check (Type.equals(t1, typ_bool)) (i,"WhileExp condition type");
		check (Type.equals(t,  typ_unit)) (i,"WhileExp type");
		t
	   end

	 | SeqExp(i, exps) =>
	   let
		val t  = #typ i
		val ts = chExps E exps
		val t1 = check' List.last ts (i,"SeqExp empty")
	   in
		check (Type.equals(t1, t)) (i,"SeqExp type");
		t
	   end

	 | CaseExp(i, exp, matchs) =>
	   let
		val t   = #typ i
		val t1  = chExp E exp
		val tts = chMatchs E matchs
	   in
		check (List.all (fn(t2,t3) =>
			Type.equals(t2,t1) andalso Type.equals(t3,t)) tts)
		      (i,"CaseExp match types");
		t
	   end

	 | RaiseExp(i, exp) =>
	   let
		val t1 = chExp E exp
	   in
		check (Type.equals(t1, typ_exn)) (i,"RaiseExp argument type");
		#typ i
	   end

	 | HandleExp(i, exp, matchs) =>
	   let
		val t   = chExp E exp
		val tts = chMatchs E matchs
	   in
		check (Type.equals(t, #typ i)) (i,"HandleExp type");
		check (List.all (fn(t1,t2) =>
			Type.equals(t1,typ_exn) andalso Type.equals(t2,t)) tts)
		      (i,"HandleExp match types");
		t
	   end

	 | FailExp(i) =>
		#typ i

	 | LazyExp(i, exp) =>
	   let
		val t = chExp E exp
	   in
		check (Type.equals(t, #typ i)) (i,"LazyExp type");
		t
	   end

	 | LetExp(i, decs, exp) =>
	   let
		val _ = Env.insertScope E
		val _ = chDecs E decs
		val t = chExp E exp
		val _ = Env.deleteScope E
	   in
		check (Type.equals(t, #typ i)) (i,"LetExp type");
		t
	   end

	 | UpExp(i, exp) =>
	   let
		val t = chExp E exp
	   in
		check (matches(t, #typ i)) (i,"UpExp type");
		t
	   end


    and chMatchs E matchs = List.map (chMatch E) matchs
    and chMatch E (Match(i, pat, exp)) =
	let
	    val _  = Env.insertScope E
	    val t1 = chPat E pat
	    val t2 = chExp E exp
	    val _  = Env.deleteScope E
	in
	    (t1,t2)
	end


  (* Patterns *)

    and chPats E pats = List.map (chPat E) pats
    and chPat E =
	fn JokPat(i) =>
		#typ i

	 | LitPat(i, lit) =>
	   let
		val t = chLit lit
	   in
		check (Type.equals(t, #typ i)) (i,"LitPat type");
		t
	   end

	 | VarPat(i, id) =>
	   let
		val t1 = chBindId E id
		val t  = #typ i
	   in
		check (Type.matches(t1,t)) (i,"VarPat type");
		t
	   end

	 | TagPat(i, lab, isNary) =>
	   let
		val  a       = chLab E lab
		val  t       = #typ i
		val (ts2,t2) = asArrows(unquantify t)
		val  r       = check' Type.asSum' t2      (i,"TagPat type")
		val  ts1     = check' Type.lookupRow(r,a) (i,"TagPat label")
		(*UNFINISHED: check isNary *)
	   in
		check (Vector.length ts1 = List.length ts2) (i,"TagPat arity");
		check (VectorPair.all Type.equals (ts1, Vector.fromList ts2))
						(i,"TagPat argument type");
		t
	   end

	 | ConPat(i, longid, isNary) =>
	   let
		val t1 = chLongid E longid
		val t  = #typ i
		(*UNFINISHED: check isNary *)
	   in
		check (Type.matches(t1,t)) (i,"ConPat type");
		check (isOpenType t)       (i,"ConPat type constructor");
		t
	   end

	 | RefPat(i) =>
	   let
		val t  = #typ i
		val t1 = check' (#2 o Type.asArrow') t (i,"RefPat type")
		val c  = check' Type.asCon' t1         (i,"RefPat type")
	   in
		check (c = con_ref) (i,"RefPat type constructor");
		t
	   end

	 | TupPat(i, pats) =>
	   let
		val t = Type.inTuple(Vector.fromList(chPats E pats))
	   in
		check (Type.equals(t, #typ i)) (i,"TupPat type");
		t
	   end

	 | ProdPat(i, fields) =>
	   let
		val t = Type.inProd(chFields chPat E fields)
	   in
		check (Type.equals(t, #typ i)) (i,"ProdPat type");
		t
	   end

	 | VecPat(i, pats) =>
	   let
		val  ts     = chPats E pats
		val  t      = #typ i
		val (t1,t2) = check' Type.asApply' t (i,"VecPat type")
		val  c      = check' Type.asCon' t1  (i,"VecPat type")
	   in
		check (c = con_vec) (i,"VecPat type constructor");
		check (List.all (fn t' => Type.equals(t',t2)) ts)
				    (i,"VecPat component type");
		t
	   end

	 | AppPat(i, pat1, pat2) =>
	   let
		val  t        = #typ i
		val  t1       = chPat E pat1
		val  t2       = chPat E pat2
		val (t11,t12) = check' Type.asArrow' t1 (i,"AppPat func type")
	   in
		check (Type.equals(t11,t2)) (i,"AppPat argument type");
		check (Type.equals(t12, t)) (i,"AppPat result type");
		t
	   end

	 | AsPat(i, pat1, pat2) =>
	   let
		val t  = #typ i
		val t1 = chPat E pat1
		val t2 = chPat E pat2
	   in
		check (Type.equals(t1,t)) (i,"AsPat left operand type");
		check (Type.equals(t2,t)) (i,"AsPat right operand type");
		t
	   end

	 | AltPat(i, []) =>
		#typ i

	 | AltPat(i, pat1::pats) =>
	   let
		val t  = #typ i
		val _  = Env.insertScope E
		val t1 = chPat E pat1
		val E' = Env.splitScope E
		val _  = chAltPats (E,E',t) pats
		val _  = chPat E pat1	(* to finally enter bindings *)
	   in
		check (Type.equals(t1, t)) (infoPat pat1, "AltPat type");
		t
	   end

	 | NegPat(i, pat) =>
	   let
		val _ = Env.insertScope E
		val t = chPat E pat
		val _ = Env.deleteScope E
	   in
		check (Type.equals(t, #typ i)) (i,"NegPat type");
		t
	   end

	 | GuardPat(i, pat, exp) =>
	   let
		val t  = chPat E pat
		val t2 = chExp E exp
	   in
		check (Type.equals(t2, typ_bool)) (i,"GuardPat condition type");
		check (Type.equals(t, #typ i))    (i,"GuardPat type");
		t
	   end

	 | WithPat(i, pat, decs) =>
	   let
		val t = chPat E pat
	   in
		chDecs E decs;
		check (Type.equals(t, #typ i)) (i,"WithPat type");
		t
	   end

    and chAltPats (E,E',t) pats = List.map (chAltPat (E,E',t)) pats
    and chAltPat  (E,E',t) pat =
	let
	    val i   = infoPat pat
	    val _   = Env.insertScope E
	    val t'  = chPat E pat
	    val E'' = Env.splitScope E
	in
	    check (Type.equals(t',t)) (i,"AltPat type");
	    check (Env.sizeScope E' = Env.sizeScope E'') (i,"AltPat bindings");
	    Env.appiScope
		(fn(z,t1) =>
		    let val t2 = check' Env.lookupExistent(E',z)
						  (i,"AltPat bindings")
		    in check (Type.equals(t1,t2)) (i,"AltPat binding types") end
		) E'';
	    t
	end


  (* Declarations *)

    and chDecs E decs = List.app (chDec E) decs
    and chDec E =
	fn ValDec(i, pat, exp) =>
	   let
		val t1 = #typ(infoPat pat)
		val t2 = #typ(infoExp exp)
	   in
		chExp E exp;
		chPat E pat;
		checkI (Type.equals(t1,t2)) (i, "ValDec lhs/rhs type")
	   end

         | RecDec(i, decs) =>
	   let in
		chRecDecsLHS E decs;
		Env.insertScope E;
		chDecs E decs;
		Env.deleteScope E
	   end

    and chRecDecsLHS E decs = List.app (chRecDecLHS E) decs
    and chRecDecLHS E =
	fn ValDec(i, pat, exp) =>
		ignore(chPat E pat)
		(*UNFINISHED: check rec restrictions*)

         | RecDec(i, decs) =>
		chRecDecsLHS E decs


  (* Components *)

    fun check (imps, (exp,sign)) = ignore(chExp (Env.new()) exp)
	(*UNFINISHED: check import and export*)
*)
    fun check _ = ()
  end
