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

structure CodeStore :> CODE_STORE =
    struct
	open IL

	type stamp = Stamp.t

	datatype id = datatype FlatGrammar.id
	datatype idDef = datatype FlatGrammar.idDef

	datatype reg =
	    SFld of index
	  | Fld of index
	  | Arg of index
	  | Loc of index
	  | Killed of index
	withtype index = int

	type class = stamp

	structure Map = StampMap
	structure ScopedMap = MakeHashScopedImpMap(FromEqHashKey(Stamp))

	type classAttrState = (extends * implements) option ref
	type scope = reg ScopedMap.t
	type classDeclsState = classDecl list ref
	type regState =
	    scope * index ref * index ref * IL.ty list ref * index list ref
	type savedRegState = scope * index list
	type instrsState = IL.instr list ref

	val namespace: dottedname ref = ref nil
	val classes: (classAttrState * scope * classDeclsState) Map.t ref =
	    ref (Map.new ())
	val env: (class * IL.id * int * regState * instrsState) list ref =
	    ref nil

	fun className class = !namespace @ ["P" ^ Stamp.toString class]
	fun sfldName i = "V" ^ Int.toString i
	fun fldName i = "G" ^ Int.toString i

	fun init dottedname =
	    (namespace := dottedname;
	     classes := Map.new ();
	     env := [(Stamp.new (), "Main", 0,
		      (ScopedMap.new (), ref 0, ref 0,
		       ref nil, ref nil), ref nil)])

	fun defineClass (stamp, extends, implements) =
	    let
		val classAttr = SOME (extends, implements)
		val ctor =
		    Method (".ctor", (Public, Instance),
			    nil, VoidTy, (nil, false),
			    [Ldarg 0,
			     Tail, Call (true, extends, ".ctor", nil, VoidTy),
			     Ret])
	    in
		case Map.lookup (!classes, stamp) of
		    SOME (classAttrRef, _, classDeclsRef) =>
			if Option.isSome (!classAttrRef) then
			    raise Crash.Crash "CodeStore.defineClass"
			else
			    (classAttrRef := classAttr;
			     classDeclsRef := ctor::(!classDeclsRef))
		  | NONE =>
			Map.insertDisjoint (!classes, stamp,
					    (ref classAttr,
					     ScopedMap.new (),
					     ref [ctor]))
	    end

	fun defineMethod (stamp, id, args) =
	    let
		val (scope, classDeclsRef) =
		    case Map.lookup (!classes, stamp) of
			SOME (_, scope, classDeclsRef) =>
			    (scope, classDeclsRef)
		      | NONE =>
			    let
				val scope = ScopedMap.new ()
				val classDeclsRef = ref nil
			    in
				Map.insertDisjoint
				(!classes, stamp,
				 (ref NONE, scope, classDeclsRef));
				(scope, classDeclsRef)
			    end
	    in
		ScopedMap.insertScope scope;
		List.foldl (fn (idDef, i) =>
			    (case idDef of
				 IdDef (Id (_, stamp, _)) =>
				     ScopedMap.insertDisjoint (scope, stamp,
							       Arg i)
			       | Wildcard => ();
			     i + 1)) 1 args;
		env :=
		(stamp, id, List.length args,
		 (scope, ref 0, ref 0, ref nil, ref nil), ref nil)::(!env)
	    end

	fun emit instr =
	    let
		val (_, _, _, _, instrsRef) = List.hd (!env)
	    in
		instrsRef := instr::(!instrsRef)
	    end

	local
	    fun currentClosure () =
		let
		    val (stamp, _, _, _, _) = List.hd (!env)
		in
		    className stamp
		end

	    fun lookup ((_, _, _, (scope, ri, _, _, _), _)::envr, stamp) =
		(case ScopedMap.lookup (scope, stamp) of
		     SOME reg => reg
		   | NONE =>
			 let
			     val i = !ri
			     val reg = Fld i
			 in   (*--** generate SFld? *)
			     lookup (envr, stamp);
			     ScopedMap.insertDisjoint (scope, stamp, reg);
			     ri := i + 1;
			     reg
			 end)
	      | lookup (nil, stamp) =
		raise Crash.Crash ("CodeStore.lookup: " ^ Stamp.toString stamp)
	in
	    fun emitStamp stamp =
		case lookup (!env, stamp) of
		    SFld i =>
			emit (Ldsfld (currentClosure (), sfldName i,
				      System.ObjectTy))
		  | Fld i =>
			(emit (Ldarg 0);
			 emit (Ldfld (currentClosure (), fldName i,
				      System.ObjectTy)))
		  | Loc i => emit (Ldloc i)
		  | Arg i => emit (Ldarg i)
		  | Killed _ => raise Crash.Crash "CodeStore.emitStamp"
	end

	fun emitId (Id (_, stamp, _)) =
	    (emit (Comment ("load " ^ Stamp.toString stamp));
	     emitStamp stamp)

	fun allocateLocal ty =
	    let
		val (_, _, _, (_, _, ri, tysRef, _), _) = List.hd (!env)
		val index = !ri
	    in
		tysRef := ty::(!tysRef); ri := index + 1; index
	    end

	fun declareLocal (IdDef (Id (_, stamp, _))) =
	    let
		val (_, _, _, (scope, _, ri, tysRef, indicesRef), _) =
		    List.hd (!env)
	    in
		emit (Comment ("store " ^ Stamp.toString stamp));
		case ScopedMap.lookup (scope, stamp) of
		    SOME (Loc i) => emit (Stloc i)
		  | SOME (Killed i) =>
			(ScopedMap.insert (scope, stamp, Loc i);
			 emit (Stloc i))
		  | SOME _ => raise Crash.Crash "CodeStore.declareLocal"
		  | NONE =>
			let
			    val i =
				case indicesRef of
				    ref nil =>
					!ri before
					(ri := !ri + 1;
					 tysRef := System.ObjectTy::(!tysRef))
				  | ref (index::rest) =>
					index before indicesRef := rest
			in
			    ScopedMap.insertDisjoint (scope, stamp, Loc i);
			    emit (Stloc i)
			end
	    end
	  | declareLocal Wildcard = emit Pop

	fun kill set =
	    let
		val (_, _, _, (scope, _, _, _, indicesRef), _) = List.hd (!env)
	    in
		StampSet.app
		(fn stamp =>
		 case ScopedMap.lookup (scope, stamp) of
		     SOME (Loc i) =>
			 (emit (Comment ("kill " ^ Stamp.toString stamp));
			  indicesRef := i::(!indicesRef);
			  ScopedMap.insert (scope, stamp, Killed i))
		    | _ => emit (Comment ("nonlocal " ^ Stamp.toString stamp)))
		set
	    end

	fun saveRegState () =
	    let
		val (_, _, _, (scope, _, _, _, ref indices), _) = hd (!env)
	    in
		(ScopedMap.cloneScope scope, indices)
	    end

	fun restoreRegState (scope', indices') =
	    let
		val (_, _, _, (scope, _, _, _, indicesRef), _) = hd (!env)
	    in
		ScopedMap.appi (fn (stamp, reg) =>
				ScopedMap.insert (scope, stamp, reg)) scope';
		indicesRef := indices'
	    end

	fun args n = List.tabulate (n, fn _ => System.ObjectTy)

	fun closeMethod () =
	    case !env of
		(stamp, id, narg, (scope, _, _, ref tys, _),
		 ref instrs)::envr =>
		    let
			val (_, _, classDeclsRef) =
			    Map.lookupExistent (!classes, stamp)
			val delta = ScopedMap.splitScope scope
			val className' = className stamp
			val _ = env := envr
			val method =
			    Method (id, (Public, Virtual),
				    args narg, System.ObjectTy,
				    (List.rev tys, false), List.rev instrs)
			val newClassDecls =
			    ScopedMap.foldi
			    (fn (stamp, reg, classDecls) =>
			     case reg of
				 SFld i =>
				     (emit Dup;
				      emitStamp stamp;
				      emit (Stsfld (className', sfldName i,
						    System.ObjectTy));
				      ScopedMap.insertDisjoint (scope, stamp,
								reg);
				      Field (sfldName i, (Public, true, false),
					     System.ObjectTy)::classDecls)
			       | Fld i =>
				     (emit Dup;
				      emitStamp stamp;
				      emit (Stfld (className', fldName i,
						   System.ObjectTy));
				      ScopedMap.insertDisjoint (scope, stamp,
								reg);
				      Field (fldName i, (Public, false, false),
					     System.ObjectTy)::classDecls)
			   | _ => classDecls) (!classDeclsRef) delta
		    in
			classDeclsRef := method::newClassDecls
		    end
	      | nil => raise Crash.Crash "CodeStore.closeMethod"

	fun close () =
	    let
		val mainMethod =
		    case !env of
			[(_, id, 0, (_, _, _, ref tys, _), ref instrs)] =>
			    Class (["Execute"], (true, SealedClass),
				   System.Object, nil,
				   [Method (id, (Public, Static),
					    [System.ObjectTy], System.ObjectTy,
					    (List.rev tys, false),
					    List.rev instrs)])
		      | _ => raise Crash.Crash "CodeStore.close"
	    in
		Map.foldi
		(fn (stamp, (ref classAttr, scope, ref classDecls), program) =>
		 let
		     val (extends, implements) = Option.valOf classAttr
		 in
		     Class (className stamp, (true, SealedClass),
			    extends, implements, classDecls)::program
		 end) [mainMethod] (!classes)
	    end
    end
