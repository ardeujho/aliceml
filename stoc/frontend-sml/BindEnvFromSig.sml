structure BindEnvFromSig :> BIND_ENV_FROM_SIG =
  struct

    open BindEnv


    (*
     * Fixity modes not available in SML are converted as follows:
     * - non-associative infix -> left-associative infix
     * - prefix  -> nonfix
     * - postfix -> nonfix
     *)

    fun assocFromAssoc(Fixity.LEFT)	= Infix.LEFT
      | assocFromAssoc(Fixity.RIGHT)	= Infix.RIGHT
      | assocFromAssoc(Fixity.NEITHER)	= Infix.LEFT

    fun infFromFix(Fixity.NONFIX)	= NONE
      | infFromFix(Fixity.PREFIX n)	= NONE
      | infFromFix(Fixity.POSTFIX n)	= NONE
      | infFromFix(Fixity.INFIX(n,a))	= SOME(assocFromAssoc a, n)

    (*
     * Arity of constructors is `approximated' through its type.
     * Reference constructors are recognized through there type as well.
     *)

    fun arity t =
	if Type.isArrow t then
	    arity(#2(Type.asArrow t)) + 1
	else
	    0

    fun isRef t =
	#3(Type.asCon(#1(Type.asApply t))) = PreboundType.path_ref
	handle Type.Type => false

    fun idStatusFromSort(Inf.VALUE, t)       = V
      | idStatusFromSort(Inf.CONSTRUCTOR, t) =
	if not(Type.isArrow t) then
	    C 0
	else
	    let
		val (t1,t2) = Type.asArrow t
	    in
		if Type.isArrow t2 then
		    C(arity t2 + 1)
		else if Type.isTuple t1 then
		    C(List.length(Type.asTuple t1))
		else if isRef t2 then
		    R
		else
		    C 1
	    end


    fun envFromInf(I,j) =
	if Inf.isTop j orelse Inf.isCon j then
	    new()
	else if Inf.isSig j then
	    envFromSig(I, Inf.asSig j)
	else if Inf.isArrow j then
	    envFromInf(I, #3(Inf.asArrow j))
	else if Inf.isLambda j then
	    envFromInf(I, #3(Inf.asLambda j))
	else if Inf.isApply j then
	    envFromInf(I, #1(Inf.asApply j))
	else
	    raise Crash.Crash "BindEnvFromSig.envFromInf: unknown interface"

    and envFromSig(I,s) =
	let
	    val E = new()
	in
	    List.app (insertItem(E,I)) (Inf.items s);
	    E
	end

    and insertItem (E,I) item =
	if Inf.isFixItem item then
	    let
		val (l,f) = Inf.asFixItem item
		val  vid  = VId.fromString(Label.toString l)
		val  is   = infFromFix f
	    in
		insertInf(E, vid, (I,is))
	    end
	else if Inf.isValItem item then
	    let
		val (l,t,w,_) = Inf.asValItem item
		val  vid      = VId.fromString(Label.toString l)
		val  x        = Stamp.new()
		val  is       = idStatusFromSort(w,t)
	    in
		insertVal(E, vid, (I,x,is))
	    end
	else if Inf.isTypItem item then
	    let
		val (l,k,w,_) = Inf.asTypItem item
		val  tycon    = TyCon.fromString(Label.toString l)
		val  x        = Stamp.new()
		val  E'       = new()
	    in
		insertTy(E, tycon, (I,x,E'))
	    end
	else if Inf.isModItem item then
	    let
		val (l,j,_) = Inf.asModItem item
		val  strid  = StrId.fromString(Label.toString l)
		val  x      = Stamp.new()
		val  E'     = envFromInf(I, j)
	    in
		insertStr(E, strid, (I,x,E'))
	    end
	else if Inf.isInfItem item then
	    let
		val (l,k,jo) = Inf.asInfItem item
		val  sigid   = SigId.fromString(Label.toString l)
		val  x       = Stamp.new()
		val  E'      = envFromInf(I, Option.getOpt(jo, Inf.inTop()))
	    in
		insertSig(E, sigid, (I,x,E'))
	    end
	else
	     raise Crash.Crash "BindEnvFromSig.insertItem: unknown item"

  end
