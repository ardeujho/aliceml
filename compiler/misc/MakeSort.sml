(*
 * Author:
 *   Leif Kornstaedt <kornstae@ps.uni-sb.de>
 *
 * Copyright:
 *   Leif Kornstaedt, 2000
 *
 * Last change:
 *   $Date$ by $Author$
 *   $Revision$
 *)

functor MakeSort(type 'a t val compare: 'a t * 'a t -> order) :> SORT
    where type 'a t = 'a t =
    struct
	type 'a t = 'a t

	fun split nil = (nil, nil)
	  | split (xs as [_]) = (xs, nil)
	  | split (x1::x2::xr) =
	    let
		val (xr1, xr2) = split xr
	    in
		(x1::xr1, x2::xr2)
	    end

	fun merge (xs as x::xr, ys as y::yr) =
	    (case compare (x, y) of
		 LESS => x::merge (xr, ys)
	       | EQUAL => x::y::merge (xr, yr)
	       | GREATER => y::merge (xs, yr))
	  | merge (nil, ys) = ys
	  | merge (xs, nil) = xs

	fun sort nil = nil
	  | sort (xs as [_]) = xs
	  | sort xs =
	    let
		val (ys, zs) = split xs
	    in
		merge (sort ys, sort zs)
	    end
    end
