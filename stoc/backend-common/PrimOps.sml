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

structure PrimOps :> PRIM_OPS =
    struct
	open FlatGrammar

	structure StringMap = MakeHashImpMap(StringHashKey)

	val map: arity option StringMap.t =
	    let
		val map = StringMap.new ()
		fun ins (name, arityOpt) =
		    StringMap.insertDisjoint (map, name, arityOpt)
	    in
		ins ("=", SOME (TupArity 2));
		ins ("<>", SOME (TupArity 2));
		ins ("Array.array", SOME (TupArity 2));
		ins ("Array.fromList", SOME Unary);
		ins ("Array.length", SOME Unary);
		ins ("Array.maxLen", NONE);
		ins ("Array.sub", SOME (TupArity 2));
		ins ("Array.update", SOME (TupArity 3));
		ins ("Char.<", SOME (TupArity 2));
		ins ("Char.>", SOME (TupArity 2));
		ins ("Char.<=", SOME (TupArity 2));
		ins ("Char.>=", SOME (TupArity 2));
		ins ("Char.ord", SOME Unary);
		ins ("Char.chr", SOME Unary);
		ins ("Char.isAlpha", SOME Unary);
		ins ("Char.isAlphaNum", SOME Unary);
		ins ("Char.isCntrl", SOME Unary);
		ins ("Char.isDigit", SOME Unary);
		ins ("Char.isGraph", SOME Unary);
		ins ("Char.isHexDigit", SOME Unary);
		ins ("Char.isLower", SOME Unary);
		ins ("Char.isPrint", SOME Unary);
		ins ("Char.isPunct", SOME Unary);
		ins ("Char.isSpace", SOME Unary);
		ins ("Char.isUpper", SOME Unary);
		ins ("Char.toLower", SOME Unary);
		ins ("Char.toUpper", SOME Unary);
		ins ("Future.Future", NONE);
		ins ("Future.alarm'", SOME Unary);
		ins ("Future.await", SOME Unary);
		ins ("Future.awaitOne", SOME (TupArity 2));
		ins ("Future.byneed", SOME Unary);
		ins ("Future.concur", SOME Unary);
		ins ("Future.isFailed", SOME Unary);
		ins ("Future.isFuture", SOME Unary);
		ins ("General.:=", SOME (TupArity 2));
		ins ("General.Chr", NONE);
		ins ("General.Div", NONE);
		ins ("General.Domain", NONE);
		ins ("General.Fail", NONE);
		ins ("General.Overflow", NONE);
		ins ("General.Size", NONE);
		ins ("General.Span", NONE);
		ins ("General.Subscript", NONE);
		ins ("General.exchange", SOME (TupArity 3));
		ins ("General.exnName", SOME Unary);
		ins ("GlobalStamp.new", SOME Unary);
		ins ("GlobalStamp.fromString", SOME Unary);
		ins ("GlobalStamp.toString", SOME Unary);
		ins ("GlobalStamp.compare", SOME (TupArity 2));
		ins ("GlobalStamp.hash", SOME Unary);
		ins ("Hole.Hole", NONE);
		ins ("Hole.fail", SOME (TupArity 2));
		ins ("Hole.fill", SOME (TupArity 2));
		ins ("Hole.future", SOME Unary);
		ins ("Hole.hole", SOME Unary);
		ins ("Hole.isFailed", SOME Unary);
		ins ("Hole.isHole", SOME Unary);
		ins ("Int.~", SOME (TupArity 2));
		ins ("Int.+", SOME (TupArity 2));
		ins ("Int.-", SOME (TupArity 2));
		ins ("Int.*", SOME (TupArity 2));
		ins ("Int.<", SOME (TupArity 2));
		ins ("Int.>", SOME (TupArity 2));
		ins ("Int.<=", SOME (TupArity 2));
		ins ("Int.>=", SOME (TupArity 2));
		ins ("Int.abs", SOME Unary);
		ins ("Int.compare", SOME (TupArity 2));
		ins ("Int.div", SOME (TupArity 2));
		ins ("Int.maxInt", NONE);
		ins ("Int.minInt", NONE);
		ins ("Int.mod", SOME (TupArity 2));
		ins ("Int.quot", SOME (TupArity 2));
		ins ("Int.rem", SOME (TupArity 2));
		ins ("Int.precision", NONE);
		ins ("Int.toString", SOME Unary);
		ins ("List.Empty", NONE);
		ins ("Math.acos", SOME Unary);
		ins ("Math.acosh", SOME Unary);
		ins ("Math.asin", SOME Unary);
		ins ("Math.asinh", SOME Unary);
		ins ("Math.atan", SOME Unary);
		ins ("Math.atanh", SOME Unary);
		ins ("Math.atan2", SOME Unary);
		ins ("Math.cos", SOME Unary);
		ins ("Math.cosh", SOME Unary);
		ins ("Math.e", NONE);
		ins ("Math.exp", SOME Unary);
		ins ("Math.ln", SOME Unary);
		ins ("Math.pi", NONE);
		ins ("Math.pow", SOME (TupArity 2));
		ins ("Math.sin", SOME Unary);
		ins ("Math.sinh", SOME Unary);
		ins ("Math.sqrt", SOME Unary);
		ins ("Math.tan", SOME Unary);
		ins ("Math.tanh", SOME Unary);
		ins ("Option.Option", NONE);
		ins ("Real.~", SOME (TupArity 2));
		ins ("Real.+", SOME (TupArity 2));
		ins ("Real.-", SOME (TupArity 2));
		ins ("Real.*", SOME (TupArity 2));
		ins ("Real./", SOME (TupArity 2));
		ins ("Real.<", SOME (TupArity 2));
		ins ("Real.>", SOME (TupArity 2));
		ins ("Real.<=", SOME (TupArity 2));
		ins ("Real.>=", SOME (TupArity 2));
		ins ("Real.ceil", SOME Unary);
		ins ("Real.compare", SOME (TupArity 2));
		ins ("Real.floor", SOME Unary);
		ins ("Real.fromInt", SOME Unary);
		ins ("Real.negInf", NONE);
		ins ("Real.posInf", NONE);
		ins ("Real.precision", NONE);
		ins ("Real.realCeil", SOME Unary);
		ins ("Real.realFloor", SOME Unary);
		ins ("Real.realRound", SOME Unary);
		ins ("Real.realTrunc", SOME Unary);
		ins ("Real.rem", SOME (TupArity 2));
		ins ("Real.round", SOME Unary);
		ins ("Real.toString", SOME Unary);
		ins ("Real.trunc", SOME Unary);
		ins ("String.^", SOME (TupArity 2));
		ins ("String.<", SOME (TupArity 2));
		ins ("String.>", SOME (TupArity 2));
		ins ("String.<=", SOME (TupArity 2));
		ins ("String.>=", SOME (TupArity 2));
		ins ("String.compare", SOME (TupArity 2));
		ins ("String.explode", SOME Unary);
		ins ("String.implode", SOME Unary);
		ins ("String.maxSize", NONE);
		ins ("String.size", SOME Unary);
		ins ("String.sub", SOME (TupArity 2));
		ins ("String.substring", SOME (TupArity 3));
		ins ("String.str", SOME Unary);
		ins ("Thread.Terminate", NONE);
		ins ("Thread.current", SOME Unary);
		ins ("Thread.isSuspended", SOME Unary);
		ins ("Thread.raiseIn", SOME (TupArity 2));
		ins ("Thread.resume", SOME Unary);
		ins ("Thread.state", SOME Unary);
		ins ("Thread.suspend", SOME Unary);
		ins ("Thread.yield", SOME Unary);
		ins ("Unsafe.Array.sub", SOME (TupArity 2));
		ins ("Unsafe.Array.update", SOME (TupArity 3));
		ins ("Unsafe.String.sub", SOME (TupArity 2));
		ins ("Unsafe.Vector.sub", SOME (TupArity 2));
		ins ("Unsafe.cast", SOME Unary);
		ins ("Vector.fromList", SOME Unary);
		ins ("Vector.maxLen", NONE);
		ins ("Vector.length", SOME Unary);
		ins ("Vector.sub", SOME (TupArity 2));
		ins ("Word.fromInt'", SOME Unary);
		ins ("Word.toInt", SOME Unary);
		ins ("Word.toIntX", SOME Unary);
		ins ("Word.+", SOME (TupArity 2));
		ins ("Word.-", SOME (TupArity 2));
		ins ("Word.*", SOME (TupArity 2));
		ins ("Word.div", SOME (TupArity 2));
		ins ("Word.mod", SOME (TupArity 2));
		ins ("Word.orb", SOME (TupArity 2));
		ins ("Word.xorb", SOME (TupArity 2));
		ins ("Word.andb", SOME (TupArity 2));
		ins ("Word.notb", SOME Unary);
		ins ("Word.<<", SOME (TupArity 2));
		ins ("Word.>>", SOME (TupArity 2));
		ins ("Word.~>>", SOME (TupArity 2));
		ins ("Word.toString", SOME Unary);
		ins ("Word.wordSize", NONE);
(*--**DEBUG*)
		ins ("OS.Process.exit", SOME Unary);
		ins ("TextIO.print", SOME Unary);
		map
	    end

	fun getArity name = StringMap.lookupExistent (map, name)
    end
