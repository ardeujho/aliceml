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

signature SIMPLIFY_REC =
    sig
	structure I: INTERMEDIATE_GRAMMAR = IntermediateGrammar

	type constraint = I.longid * I.longid
	type binding = I.id * I.exp
	type alias = I.id * I.id * I.exp_info

	val derec: I.dec list -> constraint list * binding list * alias list
    end
