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

signature IMPERATIVE_PHASE = 
    sig
	structure I: SIMPLIFIED_GRAMMAR = SimplifiedGrammar
	structure O: IMPERATIVE_GRAMMAR = ImperativeGrammar

	val translateProgram: I.dec list -> O.body
    end
