(*
 * Authors:
 *   Thorsten Brunklaus <brunklaus@ps.uni-sb.de>
 *
 * Copyright:
 *   Thorsten Brunklaus, 2000
 *
 * Last Change:
 *   $Date$ by $Author$
 *   $Revision$
 *
 *)

signature EXPLORER_COMPONENT =
    sig
	signature EXPLORER =
	    sig
		val exploreOne : (unit -> 'a) -> unit
		val exploreOneBAB : (unit -> 'a) * ('a * 'a -> unit) -> unit
		val exploreBest : (unit -> 'a) * ('a * 'a -> unit) -> unit
		val exploreAll : (unit -> 'a) -> unit
	    end

	structure Explorer : EXPLORER
    end
