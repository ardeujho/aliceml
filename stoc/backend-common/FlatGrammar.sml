(*
 * Author:
 *   Leif Kornstaedt <kornstae@ps.uni-sb.de>
 *
 * Copyright:
 *   Leif Kornstaedt, 1999-2000
 *
 * Last change:
 *   $Date$ by $Author$
 *   $Revision$
 *)

structure FlatGrammar: FLAT_GRAMMAR =
    (*--** the above signature constraint should be opaque, but SML/NJ bombs *)
    struct
	(* Annotations *)

	datatype livenessInfo =
	    Unknown
	  | LoopStart   (* internal *)
	  | LoopEnd   (* internal *)
	  | Use of StampSet.t   (* internal *)
	  | Kill of StampSet.t

	type id_info = {region: Source.region}
	type stm_info = {region: Source.region, liveness: livenessInfo ref}
	type exp_info = {region: Source.region}

	(* Statements and Expressions *)

	datatype lit = datatype IntermediateGrammar.lit

	type stamp = Stamp.t
	type name = Name.t
	type label = Label.t

	datatype id = datatype IntermediateGrammar.id

	datatype idDef =
	    IdDef of id
	  | Wildcard

	datatype funFlag =
	    PrintName of string
	  | AuxiliaryOf of stamp
	  | IsToplevel

	datatype con =
	    Con of id
	  | StaticCon of stamp

	datatype arity =
	    Unary
	  | TupArity of int
	  | ProdArity of label list
	    (* sorted, all labels distinct, no tuple *)

	type conArity = arity option

	datatype 'a args =
	    OneArg of 'a
	  | TupArgs of 'a list
	  | ProdArgs of (label * 'a) list
	    (* sorted, all labels distinct, no tuple *)

	type 'a conArgs = 'a args option

	datatype stm =
	  (* the following may never be last *)
	    ValDec of stm_info * idDef * exp
	  | RecDec of stm_info * (idDef * exp) list
	    (* all ids distinct *)
	  | RefAppDec of stm_info * idDef * id
	  | TupDec of stm_info * idDef list * id
	  | ProdDec of stm_info * (label * idDef) list * id
	  (* the following must always be last *)
	  | RaiseStm of stm_info * id
	  | ReraiseStm of stm_info * id
	  | HandleStm of stm_info * body * idDef * body * body * stamp
	  | EndHandleStm of stm_info * stamp
	  | TestStm of stm_info * id * tests * body
	  | SharedStm of stm_info * body * stamp   (* used at least twice *)
	  | ReturnStm of stm_info * exp
	  | IndirectStm of stm_info * body option ref
	  | ExportStm of stm_info * exp
	and tests =
	    LitTests of (lit * body) list
	  | TagTests of (label * int * idDef args option * body) list
	  | ConTests of (con * idDef args option * body) list
	  | VecTests of (idDef list * body) list
	and exp =
	    LitExp of exp_info * lit
	  | PrimExp of exp_info * string
	  | NewExp of exp_info * conArity
	  | VarExp of exp_info * id
	  | TagExp of exp_info * label * int * conArity
	  | ConExp of exp_info * con * conArity
	  | RefExp of exp_info
	  | TupExp of exp_info * id list
	  | ProdExp of exp_info * (label * id) list
	    (* sorted, all labels distinct, no tuple *)
	  | SelExp of exp_info * label * int
	  | VecExp of exp_info * id list
	  | FunExp of exp_info * stamp * funFlag list * idDef args * body
	  | PrimAppExp of exp_info * string * id list
	  | VarAppExp of exp_info * id * id args
	  | TagAppExp of exp_info * label * int * id args
	  | ConAppExp of exp_info * con * id args
	  | RefAppExp of exp_info * id
	  | SelAppExp of exp_info * label * int * id
	  | FunAppExp of exp_info * id * stamp * id args
	withtype body = stm list

	type sign = IntermediateGrammar.sign
	type component = (idDef * sign * Url.t) list * (body * sign)
	type t = component

	fun infoStm (ValDec (info, _, _)) = info
	  | infoStm (RecDec (info, _)) = info
	  | infoStm (RefAppDec (info, _, _)) = info
	  | infoStm (TupDec (info, _, _)) = info
	  | infoStm (ProdDec (info, _, _)) = info
	  | infoStm (RaiseStm (info, _)) = info
	  | infoStm (ReraiseStm (info, _)) = info
	  | infoStm (HandleStm (info, _, _, _, _, _)) = info
	  | infoStm (EndHandleStm (info, _)) = info
	  | infoStm (TestStm (info, _, _, _)) = info
	  | infoStm (SharedStm (info, _, _)) = info
	  | infoStm (ReturnStm (info, _)) = info
	  | infoStm (IndirectStm (info, _)) = info
	  | infoStm (ExportStm (info, _)) = info
    end
