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

signature OS_COMPONENT =
    sig
	signature OS_PROCESS =
	    sig
		eqtype status

		val success: status
		val failure: status
		val system: string -> status
		val exit: status -> 'a
		val getEnv: string -> string option
	    end

	signature OS =
	    sig
		structure Process: OS_PROCESS
	    end

	structure OS: OS
    end
