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

functor MakeLabelSort(type 'a t val get: 'a t -> Label.t) :> LABEL_SORT
    where type 'a t = 'a t =
    struct
	type 'a t = 'a t

	structure Sort =
	    MakeSort (type 'a t = 'a t
		      fun compare (x1, x2) = Label.compare (get x1, get x2))

	datatype arity =
	    Tup of int
	  | Prod

	fun isTuple (xs, i, n) =
	    if i = n then SOME n
	    else if get (Vector.sub (xs, i)) = Label.fromInt (i + 1) then
		isTuple (xs, i + 1, n)
	    else NONE

	fun sort xs =
	    let
		val xs' = Vector.fromList (Sort.sort xs)
	    in
		case isTuple (xs', 0, Vector.length xs') of
		    SOME i => (xs', Tup i)
		  | NONE => (xs', Prod)
	    end
    end
