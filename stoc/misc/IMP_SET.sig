signature IMP_SET =
  sig

    eqtype item
    type set
    type t = set

    exception Delete    of item
    exception Collision of item

    val new :		unit -> set
    val copy :		set -> set

    val delete :	set * item -> unit
    val deleteExistent:	set * item -> unit		(* Delete *)
    val insert :	set * item -> unit
    val insertDisjoint:	set * item -> unit		(* Collision *)
    val union :		set * set  -> unit
    val unionDisjoint :	set * set  -> unit		(* Collision *)

    val deleteWith :	(item -> unit) -> set * item -> unit
    val insertWith :	(item -> unit) -> set * item -> unit
    val unionWith :	(item -> unit) -> set * set  -> unit

    val member :	set * item -> bool
    val size :		set -> int
    val isEmpty :	set -> bool

    val app :		(item -> unit) -> set -> unit
    val fold :		(item * 'a -> 'a) -> 'a -> set -> 'a

  end
