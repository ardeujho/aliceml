(*
 * Standard ML label identifiers
 *
 * Definition, section 2.4
 *)


structure Lab :> LAB =
  struct

    type Lab = string
    type t   = Lab

    fun fromString s = s
    fun fromInt n    = Int.toString n
    fun toString s   = s

    fun compare(s1,s2) =
      case (Int.fromString s1, Int.fromString s2)
	of (SOME n1, SOME n2) => Int.compare(n1,n2)
	 |     _              => String.compare(s1,s2)

    fun equalsNum(s,n) =
      case Int.fromString s
	of SOME n' => n = n'
	 | NONE    => false

  end
