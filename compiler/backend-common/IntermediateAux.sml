(*
 * Author:
 *   Leif Kornstaedt <kornstae@ps.uni-sb.de>
 *
 * Copyright:
 *   Leif Kornstaedt, 1999
 *
 * Last change:
 *   $Date$ by $Author$
 *   $Revision$
 *)

structure IntermediateAux :> INTERMEDIATE_AUX =
    struct
	structure I = IntermediateGrammar
	open I

	fun freshId info = Id (info, Stamp.new (), Name.InId)

	fun idEq (Id (_, stamp1, _), Id (_, stamp2, _)) = stamp1 = stamp2

	fun occursInDec (ValDec (_, pat, exp), id) =
	    occursInPat (pat, id) orelse occursInExp (exp, id)
	  | occursInDec (RecDec (_, decs), id) =
	    List.exists (fn dec => occursInDec (dec, id)) decs
	and occursInExp (LitExp (_, _), _) = false
	  | occursInExp (PrimExp (_, _), _) = false
	  | occursInExp (NewExp (_, _, _), _) = false
	  | occursInExp (VarExp (_, ShortId (_, id)), id') = idEq (id, id')
	  | occursInExp (VarExp (_, LongId (_, _, _)), _) = false
	  | occursInExp (ConExp (_, _, _), _) = false
	  | occursInExp (RefExp _, _) = false
	  | occursInExp (TupExp (_, exps), id) =
	    List.exists (fn exp => occursInExp (exp, id)) exps
	  | occursInExp (RowExp (_, expFields), id) =
	    List.exists (fn Field (_, _, exp) => occursInExp (exp, id))
	    expFields
	  | occursInExp (SelExp (_, _), _) = false
	  | occursInExp (VecExp (_, exps), id) =
	    List.exists (fn exp => occursInExp (exp, id)) exps
	  | occursInExp (FunExp (_, matches), id) =
	    occursInMatches (matches, id)
	  | occursInExp (AppExp (_, exp1, exp2), id) =
	    occursInExp (exp1, id) orelse occursInExp (exp2, id)
	  | occursInExp (AdjExp (_, exp1, exp2), id) =
	    occursInExp (exp1, id) orelse occursInExp (exp2, id)
	  | occursInExp (UpExp (_, exp), id) = occursInExp (exp, id)
	  | occursInExp (AndExp (_, exp1, exp2), id) =
	    occursInExp (exp1, id) orelse occursInExp (exp2, id)
	  | occursInExp (OrExp (_, exp1, exp2), id) =
	    occursInExp (exp1, id) orelse occursInExp (exp2, id)
	  | occursInExp (IfExp (_, exp1, exp2, exp3), id) =
	    occursInExp (exp1, id) orelse occursInExp (exp2, id) orelse
	    occursInExp (exp3, id)
	  | occursInExp (WhileExp (_, exp1, exp2), id) =
	    occursInExp (exp1, id) orelse occursInExp (exp2, id)
	  | occursInExp (SeqExp (_, exps), id) =
	    List.exists (fn exp => occursInExp (exp, id)) exps
	  | occursInExp (CaseExp (_, exp, matches), id) =
	    occursInExp (exp, id) orelse occursInMatches (matches, id)
	  | occursInExp (RaiseExp (_, exp), id) = occursInExp (exp, id)
	  | occursInExp (HandleExp (_, exp, matches), id) =
	    occursInExp (exp, id) orelse occursInMatches (matches, id)
	  | occursInExp (LetExp (_, decs, exp), id) =
	    List.exists (fn dec => occursInDec (dec, id)) decs orelse
	    occursInExp (exp, id)
	and occursInMatches (matches, id) =
	    List.exists (fn Match (_, pat, exp) =>
		       occursInPat (pat, id) orelse occursInExp (exp, id))
	    matches
	and occursInPat (WildPat _, _) = false
	  | occursInPat (LitPat (_, _), _) = false
	  | occursInPat (VarPat (_, _), _) = false
	  | occursInPat (ConPat (_, _, _), _) = false
	  | occursInPat (RefPat _, id) = false
	  | occursInPat (TupPat (_, pats), id) =
	    List.exists (fn pat => occursInPat (pat, id)) pats
	  | occursInPat (RowPat (_, patFields), id) =
	    List.exists (fn Field (_, _, pat) => occursInPat (pat, id))
	    patFields
	  | occursInPat (VecPat (_, pats), id) =
	    List.exists (fn pat => occursInPat (pat, id)) pats
	  | occursInPat (AppPat (_, pat1, pat2), id) =
	    occursInPat (pat1, id) orelse occursInPat (pat2, id)
	  | occursInPat (AsPat (_, pat1, pat2), id) =
	    occursInPat (pat1, id) orelse occursInPat (pat2, id)
	  | occursInPat (AltPat (_, pats), id) =
	    List.exists (fn pat => occursInPat (pat, id)) pats
	  | occursInPat (NegPat (_, pat), id) = occursInPat (pat, id)
	  | occursInPat (GuardPat (_, pat, exp), id) =
	    occursInPat (pat, id) orelse occursInExp (exp, id)
	  | occursInPat (WithPat (_, pat, decs), id) =
	    occursInPat (pat, id) orelse
	    List.exists (fn dec => occursInDec (dec, id)) decs

	local
	    fun patternVariablesOf' (WildPat _, ids) = ids
	      | patternVariablesOf' (LitPat (_, _), ids) = ids
	      | patternVariablesOf' (VarPat (_, id), ids) = id::ids
	      | patternVariablesOf' (ConPat (_, _, _), ids) = ids
	      | patternVariablesOf' (RefPat _, ids) = ids
	      | patternVariablesOf' (TupPat (_, pats), ids) =
		foldr patternVariablesOf' ids pats
	      | patternVariablesOf' (RowPat (_, fieldPats), ids) =
		foldr (fn (Field (_, _, pat), ids) =>
		       patternVariablesOf' (pat, ids)) ids fieldPats
	      | patternVariablesOf' (VecPat (_, pats), ids) =
		foldr patternVariablesOf' ids pats
	      | patternVariablesOf' (AppPat (_, pat1, pat2), ids) =
		patternVariablesOf' (pat1, patternVariablesOf' (pat2, ids))
	      | patternVariablesOf' (AsPat (_, pat1, pat2), ids) =
		patternVariablesOf' (pat1, patternVariablesOf' (pat2, ids))
	      | patternVariablesOf' (AltPat (_, pat::_), ids) =
		patternVariablesOf' (pat, ids)
	      | patternVariablesOf' (AltPat (_, nil), ids) = ids
	      | patternVariablesOf' (NegPat (_, _), ids) = ids
	      | patternVariablesOf' (GuardPat (_, pat, _), ids) =
		patternVariablesOf' (pat, ids)
	      | patternVariablesOf' (WithPat (_, pat, decs), ids) =
		patternVariablesOf' (pat, foldr declaredVariables ids decs)
	    and declaredVariables (ValDec (_, pat, _), ids) =
		patternVariablesOf' (pat, ids)
	      | declaredVariables (RecDec (_, decs), ids) =
		foldr declaredVariables ids decs
	in
	    fun patternVariablesOf pat = patternVariablesOf' (pat, nil)
	end

	type subst = (id * id) list

	fun lookup ((Id (_, stamp, _), id')::subst, id0 as Id (_, stamp0, _)) =
	    if stamp = stamp0 then id'
	    else lookup (subst, id0)
	  | lookup (nil, id0) = id0

	fun substLongId (ShortId (info, id), subst) =
	    ShortId (info, lookup (subst, id))
	  | substLongId (longid as LongId (_, _, _), _) = longid

	fun substDecs (dec::decr, subst) =
	    substDec (dec, subst)::substDecs (decr, subst)
	  | substDecs (nil, _) = nil
	and substDec (ValDec (info, pat, exp), subst) =
	    ValDec (info, substPat (pat, subst), substExp (exp, subst))
	  | substDec (RecDec (info, decs), subst) =
	    RecDec (info, List.map (fn dec => substDec (dec, subst)) decs)
	and substExp (exp as LitExp (_, _), _) = exp
	  | substExp (exp as PrimExp (_, _), _) = exp
	  | substExp (exp as NewExp (_, _, _), _) = exp
	  | substExp (VarExp (info, longid), subst) =
	    VarExp (info, substLongId (longid, subst))
	  | substExp (exp as ConExp (_, _, _), _) = exp
	  | substExp (exp as RefExp _, _) = exp
	  | substExp (TupExp (info, exps), subst) =
	    TupExp (info, List.map (fn exp => substExp (exp, subst)) exps)
	  | substExp (RowExp (info, expFields), subst) =
	    RowExp (info,
		    List.map (fn Field (info, label, exp) =>
			      Field (info, label, substExp (exp, subst)))
		    expFields)
	  | substExp (exp as SelExp (_, _), _) = exp
	  | substExp (VecExp (info, exps), subst) =
	    VecExp (info, List.map (fn exp => substExp (exp, subst)) exps)
	  | substExp (FunExp (info, matches), subst) =
	    FunExp (info, substMatches (matches, subst))
	  | substExp (AppExp (info, exp1, exp2), subst) =
	    AppExp (info, substExp (exp1, subst), substExp (exp2, subst))
	  | substExp (AdjExp (info, exp1, exp2), subst) =
	    AdjExp (info, substExp (exp1, subst), substExp (exp2, subst))
	  | substExp (UpExp (info, exp), subst) =
	    UpExp (info, substExp (exp, subst))
	  | substExp (AndExp (info, exp1, exp2), subst) =
	    AndExp (info, substExp (exp1, subst), substExp (exp2, subst))
	  | substExp (OrExp (info, exp1, exp2), subst) =
	    OrExp (info, substExp (exp1, subst), substExp (exp2, subst))
	  | substExp (IfExp (info, exp1, exp2, exp3), subst) =
	    IfExp (info, substExp (exp1, subst),
		   substExp (exp2, subst), substExp (exp3, subst))
	  | substExp (WhileExp (info, exp1, exp2), subst) =
	    WhileExp (info, substExp (exp1, subst), substExp (exp2, subst))
	  | substExp (SeqExp (info, exps), subst) =
	    SeqExp (info, List.map (fn exp => substExp (exp, subst)) exps)
	  | substExp (CaseExp (info, exp, matches), subst) =
	    CaseExp (info, substExp (exp, subst),
		     substMatches (matches, subst))
	  | substExp (RaiseExp (info, exp), subst) =
	    RaiseExp (info, substExp (exp, subst))
	  | substExp (HandleExp (info, exp, matches), subst) =
	    HandleExp (info, substExp (exp, subst),
		       substMatches (matches, subst))
	  | substExp (LetExp (info, decs, exp), subst) =
	    LetExp (info, substDecs (decs, subst), substExp (exp, subst))
	and substMatches (matches, subst) =
	    List.map (fn Match (info, pat, exp) =>
		      Match (info, substPat (pat, subst),
			     substExp (exp, subst))) matches
	and substPat (pat as WildPat _, _) = pat
	  | substPat (pat as LitPat (_, _), _) = pat
	  | substPat (pat as VarPat (_, _), _) = pat
	  | substPat (ConPat (info, longid, isNAry), subst) =
	    ConPat (info, substLongId (longid, subst), isNAry)
	  | substPat (pat as RefPat _, subst) = pat
	  | substPat (TupPat (info, pats), subst) =
	    TupPat (info, List.map (fn pat => substPat (pat, subst)) pats)
	  | substPat (RowPat (info, patFields), subst) =
	    RowPat (info,
		    List.map (fn Field (info, label, pat) =>
			      Field (info, label, substPat (pat, subst)))
		    patFields)
	  | substPat (VecPat (info, pats), subst) =
	    VecPat (info, List.map (fn pat => substPat (pat, subst)) pats)
	  | substPat (AppPat (info, pat1, pat2), subst) =
	    AppPat (info, substPat (pat1, subst), substPat (pat2, subst))
	  | substPat (AsPat (info, pat1, pat2), subst) =
	    AsPat (info, substPat (pat1, subst), substPat (pat2, subst))
	  | substPat (AltPat (info, pats), subst) =
	    AltPat (info, List.map (fn pat => substPat (pat, subst)) pats)
	  | substPat (NegPat (info, pat), subst) =
	    NegPat (info, substPat (pat, subst))
	  | substPat (GuardPat (info, pat, exp), subst) =
	    GuardPat (info, substPat (pat, subst), substExp (exp, subst))
	  | substPat (WithPat (info, pat, decs), subst) =
	    WithPat (info, substPat (pat, subst), substDecs (decs, subst))

	(* If the same test occurs in two patterns at the same position,
	 * then these may be merged by the pattern matching compiler.
	 * In this process, a global substitution is built such that the
	 * identifiers bound at each pattern position are all mapped to
	 * common identifiers.
	 * In the presence of disjunctive patterns, such a substitution can
	 * in general only be made consistent with all pattern bindings by
	 * first uniquely renaming, then binding all the original identifiers
	 * by `with' declarations.  `with' declarations are not affected by
	 * the substitution because they are never merged.
	 *
	 * `separateAlt' moves all bindings to `with' declarations.  These
	 * are placed right at the end of each alternative pattern to allow
	 * for a maximum of merging possibilities.
	 * In principle, it is sufficient to do so only within disjunctive
	 * patterns.  If we apply this on the toplevel as well however,
	 * we need not substitute into the right hand side of a match.
	 *)

	fun separateAlt pat =
	    let
		val (pat', subst) = relax (pat, nil)
		val decs =
		    List.map
		    (fn (id, id') =>
		     let
			 val info = infoId id
			 val exp = VarExp (info, ShortId (info, id'))
		     in
			 (*--** the pattern below does not carry its type *)
			 ValDec (info, VarPat (info, id), exp)
		     end) subst
	    in
		case decs of
		    nil => pat'
		  | _::_ => WithPat (infoPat pat', pat', decs)
	    end
	and relax (pat as WildPat _, subst) = (pat, subst)
	  | relax (pat as LitPat (_, _), subst) = (pat, subst)
	  | relax (VarPat (info, id), subst) =
	    let
		val id' = freshId info
	    in
		(VarPat (info, id'), (id, id')::subst)
	    end
	  | relax (pat as ConPat (_, _, _), subst) = (pat, subst)
	  | relax (pat as RefPat _, subst) = (pat, subst)
	  | relax (TupPat (info, pats), subst) =
	    let
		val (pats', subst') =
		    List.foldr (fn (pat, (pats, subst)) =>
				let
				    val (pat', subst') = relax (pat, subst)
				in
				    (pat'::pats, subst')
				end) (nil, subst) pats
	    in
		(TupPat (info, pats'), subst')
	    end
	  | relax (RowPat (info, patFields), subst) =
	    let
		val (patFields', subst') =
		    List.foldr
		    (fn (Field (info, label, pat), (patFields, subst)) =>
		     let
			 val (pat', subst') = relax (pat, subst)
		     in
			 (Field (info, label, pat')::patFields, subst')
		     end) (nil, subst) patFields
	    in
		(RowPat (info, patFields'), subst')
	    end
	  | relax (VecPat (info, pats), subst) =
	    let
		val (pats', subst') =
		    List.foldr (fn (pat, (pats, subst)) =>
				let
				    val (pat', subst') = relax (pat, subst)
				in
				    (pat'::pats, subst')
				end) (nil, subst) pats
	    in
		(VecPat (info, pats'), subst')
	    end
	  | relax (AppPat (info, pat1, pat2), subst) =
	    let
		val (pat1', subst') = relax (pat1, subst)
		val (pat2', subst'') = relax (pat2, subst')
	    in
		(AppPat (info, pat1', pat2'), subst'')
	    end
	  | relax (AsPat (info, pat1, pat2), subst) =
	    let
		val (pat1', subst') = relax (pat1, subst)
		val (pat2', subst'') = relax (pat2, subst')
	    in
		(AsPat (info, pat1', pat2'), subst'')
	    end
	  | relax (AltPat (info, pats), subst) =
	    (AltPat (info, List.map separateAlt pats), subst)
	  | relax (NegPat (info, pat), subst) =
	    (NegPat (info, separateAlt pat), subst)
	  | relax (GuardPat (info, pat, exp), subst) =
	    let
		val (pat', subst') = relax (pat, subst)
	    in
		(GuardPat (info, pat', substExp (exp, subst')), subst')
	    end
	  | relax (WithPat (info, pat, decs), subst) =
	    let
		val (pat', subst') = relax (pat, subst)
	    in
		(WithPat (info, pat', substDecs (decs, subst')), subst')
	    end

	structure O = ImperativeGrammar
	open O

	local
	    fun parseRow row =
		if Type.isEmptyRow row then
		    if Type.isUnknownRow row then
			raise Crash.Crash "IntermediateAus.parseRow 1"
		    else nil
		else
		    case Type.headRow row of
			(label, [typ]) =>
			    (label, typ)::parseRow (Type.tailRow row)
		      | (_, _) =>
			    raise Crash.Crash "IntermediateAux.parseRow 2"
	in
	    fun makeConArity (info, isNAry) =
		let
		    val typ = IntermediateInfo.typ info
		in
		    if Type.isArrow typ then
			if isNAry then
			    let
				val (argTyp, _) = Type.asArrow typ
			    in
				if Type.isTuple argTyp then
				    Tuple (List.length (Type.asTuple argTyp))
				else if Type.isRow argTyp then
				    case LabelSort.sort
					(parseRow (Type.asRow typ)) of
					(_, LabelSort.Tup i) => Tuple i
				      | (labelTypList, LabelSort.Rec) =>
					    Record (List.map #1 labelTypList)
				     else Unary
			    end
			else Unary
		    else Nullary
		end handle IntermediateInfo.Info => Nullary
		(*--** NewExp is missing its type, therefore the `handle' *)
	end
    end
