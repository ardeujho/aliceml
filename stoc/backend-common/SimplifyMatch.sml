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

structure SimplifyMatch :> SIMPLIFY_MATCH =
    struct
	structure I = IntermediateGrammar
	structure O = FlatGrammar

	open I
	open IntermediateAux

	(* Tests *)

	datatype selector =
	    LABEL of Label.t
	  | LONGID of Stamp.t * Label.t list
	type pos = selector list

	datatype test =
	    LitTest of I.lit
	  | TagTest of Label.t * int * unit O.conArgs * O.conArity
	  | ConTest of I.longid * unit O.conArgs * O.conArity
	  | RefTest
	  | TupTest of int
	  | ProdTest of Label.t vector
	    (* sorted, all labels distinct, no tuple *)
	  | VecTest of int
	  | GuardTest of mapping * I.exp
	  | DecTest of mapping * I.dec vector
	withtype mapping = (pos * I.id) list

	(* Test Sequences *)

	datatype testSeqElem =
	    Test of pos * test
	  | Neg of testSeq
	  | Alt of testSeq vector
	withtype testSeq = testSeqElem list

	(* Test Sequence Construction *)

	local
	    fun longidToSelector' (ShortId (_, Id (_, stamp, _))) =
		(stamp, nil)
	      | longidToSelector' (LongId (_, longid, Lab (_, label))) =
		let
		    val (stamp, labels) = longidToSelector' longid
		in
		    (stamp, label::labels)
		end
	in
	    fun longidToSelector longid =
		let
		    val (stamp, labels) = longidToSelector' longid
		in
		    LONGID (stamp, List.rev labels)
		end
	end

	structure LabelSort =
	    MakeLabelSort(type 'a t = Label.t
			  fun get label = label)

	fun getRow typ =
	    if Type.isProd typ then
		let
		    val labels = rowLabels (Type.asProd typ)
		    val (labels', arity) = LabelSort.sort labels
		in
		    (labels', arity)
		end
	    else
		let
		    val n = Vector.length (Type.asTuple typ)
		in
		    (Vector.tabulate (n, fn i => Label.fromInt (i + 1)),
		     LabelSort.Tup n)
		end

	fun makeAppConArgs (TupPat (_, pats), true, pos) =
	    (Vector.mapi (fn (i, pat) =>
			  (LABEL (Label.fromInt (i + 1))::pos, pat))
	     (pats, 0, NONE),
	     SOME (O.TupArgs (Vector.map ignore pats)))
	  | makeAppConArgs (pat as ProdPat (info, patFields), true, pos) =
	    (case getRow (#typ info) of
		 (_, LabelSort.Tup n) =>
		     (Vector.map (fn Field (_, Lab (_, label), pat) =>
				  (LABEL label::pos, pat)) patFields,
		      SOME (O.TupArgs (Vector.tabulate (n, ignore))))
	       | (labels, LabelSort.Prod) =>
		     (Vector.map (fn Field (_, Lab (_, label), pat) =>
				  (LABEL label::pos, pat)) patFields,
		      SOME (O.ProdArgs (Vector.map (fn label => (label, ()))
					labels))))
	  | makeAppConArgs (pat, _, pos) =
	    if isZeroTyp (#typ (infoPat pat)) then (#[], NONE)
	    else (#[(pos, pat)], SOME (O.OneArg ()))

	fun makeTestSeq (JokPat _, _, rest, mapping) = (rest, mapping)
	  | makeTestSeq (LitPat (_, lit), pos, rest, mapping) =
	    (Test (pos, LitTest lit)::rest, mapping)
	  | makeTestSeq (VarPat (_, id), pos, rest, mapping) =
	    (rest, (pos, id)::mapping)
	  | makeTestSeq (TagPat (info, Lab (_, label), pat, isNAry),
			 pos, rest, mapping) =
	    let
		val (posPatVector, conArgs) =
		    makeAppConArgs (pat, isNAry, LABEL label::pos)
		val typ = #typ info
		val n = tagIndex (typ, label)
		val conArity = makeConArity (typ, isNAry)
	    in
		Vector.foldl (fn ((pos, pat), (rest, mapping)) =>
			      makeTestSeq (pat, pos, rest, mapping))
		(Test (pos, TagTest (label, n, conArgs, conArity))::rest,
		 mapping) posPatVector
	    end
	  | makeTestSeq (ConPat (info, longid, pat, isNAry),
			 pos, rest, mapping) =
	    let
		val (posPatVector, conArgs) =
		    makeAppConArgs (pat, isNAry, longidToSelector longid::pos)
		val conArity = makeConArity (#typ info, isNAry)
	    in
		Vector.foldl (fn ((pos, pat), (rest, mapping)) =>
			      makeTestSeq (pat, pos, rest, mapping))
		(Test (pos, ConTest (longid, conArgs, conArity))::rest,
		 mapping) posPatVector
	    end
	  | makeTestSeq (RefPat (_, pat), pos, rest, mapping) =
	    makeTestSeq (pat, LABEL (Label.fromString "ref")::pos,
			 Test (pos, RefTest)::rest, mapping)
	  | makeTestSeq (TupPat (_, pats), pos, rest, mapping) =
	    Vector.foldli
	    (fn (i, pat, (rest, mapping)) =>
	     makeTestSeq (pat, LABEL (Label.fromInt (i + 1))::pos,
			  rest, mapping))
	    (Test (pos, TupTest (Vector.length pats))::rest, mapping)
	    (pats, 0, NONE)
	  | makeTestSeq (ProdPat (info, patFields), pos, rest, mapping) =
	    Vector.foldl (fn (Field (_, Lab (_, label), pat),
			      (rest, mapping)) =>
			  makeTestSeq (pat, LABEL label::pos, rest, mapping))
	    (case getRow (#typ info) of
		 (_, LabelSort.Tup n) =>
		     Test (pos, TupTest n)::rest
	       | (labels, LabelSort.Prod) =>
		     Test (pos, ProdTest labels)::rest, mapping)
	    patFields
	  | makeTestSeq (VecPat (_, pats), pos, rest, mapping) =
	    Vector.foldli
	    (fn (i, pat, (rest, mapping)) =>
	     makeTestSeq (pat, LABEL (Label.fromInt (i + 1))::pos,
			  rest, mapping))
	    (Test (pos, VecTest (Vector.length pats))::rest, mapping)
	    (pats, 0, NONE)
	  | makeTestSeq (AsPat (_, pat1, pat2), pos, rest, mapping) =
	    let
		val (rest', mapping') = makeTestSeq (pat1, pos, rest, mapping)
	    in
		makeTestSeq (pat2, pos, rest', mapping')
	    end
	  | makeTestSeq (AltPat (_, pats), pos, rest, mapping) =
	    (Alt (Vector.map (fn pat =>
			      let
				  val (rest', _) =
				      makeTestSeq (pat, pos, nil, mapping)
			      in
				  List.rev rest'
			      end) pats)::rest, mapping)
	  | makeTestSeq (NegPat (_, pat), pos, rest, mapping) =
	    let
		val (rest', _) = makeTestSeq (pat, pos, nil, mapping)
	    in
		(Neg (List.rev rest')::rest, mapping)
	    end
	  | makeTestSeq (GuardPat (_, pat, exp), pos, rest, mapping) =
	    let
		val (rest', mapping') = makeTestSeq (pat, pos, rest, mapping)
	    in
		(Test (pos, GuardTest (mapping', exp))::rest', mapping')
	    end
	  | makeTestSeq (WithPat (_, pat, decs), pos, rest, mapping) =
	    let
		val (rest', mapping') = makeTestSeq (pat, pos, rest, mapping)
	    in
		(Test (pos, DecTest (mapping', decs))::rest', mapping')
	    end

	(* Test Graphs *)

	datatype testGraph =
	    Node of pos * test * testGraph ref * testGraph ref * nodeStatus ref
	  | Leaf of O.body * O.body option ref
	  | Default
	and nodeStatus =
	    Initial
	  | Raw of testGraph list * testGraph list
	    (* list of all nodes that reference this node as `then',
	     * list of all nodes that reference this node as `else' *)
	  | Cooked of (pos * test) list * (pos * test) list
	    (* set of true tests for this node's `then',
	     * set of false tests for this node's `else' *)
	  | Translated of O.body

	(* Debugging *)

	fun posToString' (LABEL l::rest) =
	    Label.toString l ^ "." ^ posToString' rest
	  | posToString' (LONGID _::rest) =
	    "<longid>." ^ posToString' rest
	  | posToString' nil = "<e>"

	fun posToString pos = posToString' (List.rev pos)

	fun indent 0 = ""
	  | indent n = "  " ^ indent (n - 1)

	fun litToString (WordLit w) = LargeWord.toString w
	  | litToString (IntLit i) = LargeInt.toString i
	  | litToString (CharLit c) = "#\"" ^ WideChar.toString c ^ "\""
	  | litToString (StringLit s) = "\"" ^ s ^ "\""
	  | litToString (RealLit s) = s

	fun testToString (LitTest lit) = "lit " ^ litToString lit
	  | testToString (TagTest (label, n, NONE, _)) =
	    "tag0 " ^ Label.toString label ^ "/" ^ Int.toString n
	  | testToString (TagTest (label, n, SOME (O.OneArg _), _)) =
	    "tag " ^ Label.toString label ^ "/" ^ Int.toString n ^ " one"
	  | testToString (TagTest (label, n, SOME (O.TupArgs xs), _)) =
	    "tag " ^ Label.toString label ^ "/" ^ Int.toString n ^
	    " tup " ^ Int.toString (Vector.length xs)
	  | testToString (TagTest (label, n, SOME (O.ProdArgs _), _)) =
	    "tag " ^ Label.toString label ^ "/" ^ Int.toString n ^ " prod"
	  | testToString (ConTest (_, NONE, _)) = "con0"
	  | testToString (ConTest (_, SOME (O.OneArg _), _)) = "con"
	  | testToString (ConTest (_, SOME (O.TupArgs xs), _)) =
	    "con tup " ^ Int.toString (Vector.length xs)
	  | testToString (ConTest (_, SOME (O.ProdArgs _), _)) = "con prod"
	  | testToString RefTest = "ref"
	  | testToString (TupTest n) = "tup " ^ Int.toString n
	  | testToString (ProdTest labelTyplist) = "prod"
	  | testToString (VecTest n) = "vec " ^ Int.toString n
	  | testToString (GuardTest (_, _)) = "guard"
	  | testToString (DecTest (_, decs)) =
	    "dec " ^ Int.toString (Vector.length decs)

	fun posTestListToString ((pos, test)::rest) =
	    posToString pos ^ ": " ^ testToString test ^
	    (if List.null rest then "" else ", " ^ posTestListToString rest)
	  | posTestListToString nil = ""

	fun graphToString (Node (pos, test, ref thenGraph, ref elseGraph,
				 ref status), level) =
	    indent level ^
	    posToString pos ^ ": " ^
	    testToString test ^ "\n" ^
	    (case status of
		 Cooked (posTestList, _) =>
		     indent (level + 1) ^ "[" ^
		     posTestListToString posTestList ^ "]\n"
	       | _ => "") ^
	    graphToString (thenGraph, level + 1) ^
	    (case status of
		 Cooked (_, posTestList) =>
		     indent (level + 1) ^ "[" ^
		     posTestListToString posTestList ^ "]\n"
	       | _ => "") ^
	    graphToString (elseGraph, level + 1)
	  | graphToString (Leaf (body, _), level) =
	    indent level ^ "leaf " ^
	    (Source.regionToString (#region (O.infoStm (List.hd body)))) ^ "\n"
	  | graphToString (Default, level) = indent level ^ "default\n"

	fun mappingToString' ((pos, _)::mapping) =
	    " " ^ posToString pos ^ mappingToString' mapping
	  | mappingToString' nil = ""

	fun mappingToString mapping =
	    "dom(mapping) =" ^ mappingToString' mapping ^ "\n"

	fun testSeqToString' (Test (pos, test)::rest) =
	    posToString pos ^ ": " ^ testToString test ^ "\n" ^
	    testSeqToString' rest
	  | testSeqToString' (Neg testSeq::rest) =
	    "<neg>\n" ^ testSeqToString' testSeq ^ "</neg>\n" ^
	    testSeqToString' rest
	  | testSeqToString' (Alt testSeqs::rest) =
	    Vector.foldr (fn (testSeq, s) =>
			  "<alt>\n" ^ testSeqToString' testSeq ^ s)
	    "</alt>\n" testSeqs ^
	    testSeqToString' rest
	  | testSeqToString' nil = ""

	fun testSeqToString testSeq =
	    "<seq>\n" ^ testSeqToString' testSeq ^ "</seq>\n"

	(* Construction of Backtracking Test Trees *)

	fun conArgsEq (NONE, NONE) = true
	  | conArgsEq (SOME (O.OneArg _), SOME (O.OneArg _)) = true
	  | conArgsEq (SOME (O.TupArgs _), SOME (O.TupArgs _)) = true
	  | conArgsEq (SOME (O.ProdArgs _), SOME (O.ProdArgs _)) = true
	  | conArgsEq (_, _) = false

	fun testEq (LitTest lit1, LitTest lit2) = lit1 = lit2
	  | testEq (TagTest (_, n1, conArgs1, _),
		    TagTest (_, n2, conArgs2, _)) =
	    n1 = n2 andalso conArgsEq (conArgs1, conArgs2)
	  | testEq (ConTest (longid1, conArgs1, _),
		    ConTest (longid2, conArgs2, _)) =
	    longidToSelector longid1 = longidToSelector longid2 andalso
	    conArgsEq (conArgs1, conArgs2)
	  | testEq (TupTest _, TupTest _) = true
	  | testEq (ProdTest _, ProdTest _) = true
	  | testEq (VecTest n1, VecTest n2) = n1 = n2
	  | testEq (_, _) = false

	fun areParallelTests (LitTest lit1, LitTest lit2) = lit1 <> lit2
	  | areParallelTests (VecTest n1, VecTest n2) = n1 <> n2
	  | areParallelTests (_, _) = false

	local
	    fun findTest (Node (pos', test', thenTreeRef, elseTreeRef, _),
			  pos, test) =
		if pos = pos' then
		    if testEq (test, test') then SOME thenTreeRef
		    else if areParallelTests (test, test') then
			findTest (!elseTreeRef, pos, test)
		    else NONE
		else NONE
	      | findTest (_, _, _) = NONE
	in
	    fun mergeIntoTree (Test (pos, test)::testSeqRest,
			       thenTree, elseTree) =
		(case findTest (elseTree, pos, test) of
		     SOME (treeRef as ref tree) =>
			 let
			     val newTree =
				 mergeIntoTree (testSeqRest, thenTree, tree)
			 in
			     treeRef := newTree; elseTree
			 end
		   | NONE =>
			 let
			     val newThenTree =
				 mergeIntoTree (testSeqRest, thenTree, Default)
			 in
			     Node (pos, test, ref newThenTree, ref elseTree,
				   ref Initial)
			 end)
	      | mergeIntoTree (Neg testSeq::testSeqRest, thenTree, elseTree) =
		mergeIntoTree (testSeq, elseTree,
			       mergeIntoTree (testSeqRest, thenTree, elseTree))
	      | mergeIntoTree (Alt testSeqs::testSeqRest, thenTree, elseTree) =
		(*--** this may create duplicate code in some cases. *)
		(* This could be removed by reconstructing the whole graph *)
		(* using hash-consing. *)
		Vector.foldr
		(fn (testSeq, elseTree) =>
		 mergeIntoTree (testSeq @ testSeqRest, thenTree, elseTree))
		elseTree testSeqs
	      | mergeIntoTree (nil, thenTree, _) = thenTree
	end

	(* Elimination of Backtracking, Producing a Test Graph *)

	fun propagateElses (Node (_, _, thenTreeRef, elseTreeRef, _),
			    defaultTree) =
	    (case !elseTreeRef of
		 Default => elseTreeRef := defaultTree
	       | elseTree => propagateElses (elseTree, defaultTree);
	     case !thenTreeRef of
		 Default => thenTreeRef := defaultTree
	       | thenTree => propagateElses (thenTree, !elseTreeRef))
	  | propagateElses (Leaf (_, _), _) = ()
	  | propagateElses (Default, _) =
	    raise Crash.Crash "SimplifyMatch.propagateElses"

	(* Optimization of the Test Graph *)

	(* In a first step (`Raw' annotations), compute for each node n
	 * the list of all nodes that reach n through a `then' edge, and
	 * the list of all the nodes that reach n through a `false' edge.
	 *
	 * In a second step (`Cooked' annotations), compute for each edge
	 * (i.e., reference to a graph) the set of pairs (pos * test) that
	 * are true resp. false upon taking this edge.  Use this information
	 * to shorten paths.  This is essential to maximize parallelization
	 * of tests at the same position in ValuePropagationPhase.
	 *
	 * Note: Only the `true' sets for each `then' edge and the `false'
	 * sets for each `else' edge need to be stored for subsequent
	 * computations.
	 *)

	local
	    fun union (NONE, gs) = gs
	      | union (SOME g, gr) = g::gr

	    fun computeRaw (graph as Node (_, _, ref thenGraph, ref elseGraph,
					   status as ref Initial),
			    prevTrueOpt, prevFalseOpt) =
		(status := Raw (union (prevTrueOpt, nil),
				union (prevFalseOpt, nil));
		 computeRaw (thenGraph, SOME graph, NONE);
		 computeRaw (elseGraph, NONE, SOME graph))
	      | computeRaw (Node (_, _, _, _, status as
				  ref (Raw (trueGraphs, falseGraphs))),
			    prevTrueOpt, prevFalseOpt) =
		status := Raw (union (prevTrueOpt, trueGraphs),
			       union (prevFalseOpt, falseGraphs))
	      | computeRaw (_, _, _) = ()

	    fun testSetMember (pos, test, (pos', test')::testSetRest) =
		pos = pos' andalso testEq (test, test')
		orelse testSetMember (pos, test, testSetRest)
	      | testSetMember (_, _, nil) = false

	    fun testSetIntersect ((pos, test)::testSetRest, testSet') =
		if testSetMember (pos, test, testSet') then
		    (pos, test)::testSetIntersect (testSetRest, testSet')
		else testSetIntersect (testSetRest, testSet')
	      | testSetIntersect (nil, _) = nil

	    fun disentailed (pos, test, (pos', test')::rest) =
		pos = pos' andalso areParallelTests (test, test')
		orelse disentailed (pos, test, rest)
	      | disentailed (_, _, nil) = false

	    fun optimizeNode (graphRef as ref (Node (pos, test, ref thenGraph,
						     ref elseGraph, _)),
			      trueSet, falseSet) =
		(if testSetMember (pos, test, trueSet) then
		     (graphRef := thenGraph;
		      optimizeNode (graphRef, trueSet, falseSet))
		 else if testSetMember (pos, test, falseSet)
		     orelse disentailed (pos, test, trueSet) then
		     (graphRef := elseGraph;
		      optimizeNode (graphRef, trueSet, falseSet))
		 else ())
	      | optimizeNode (ref _, _, _) = ()

	    fun getSets (Node (pos, test, thenGraphRef, elseGraphRef,
			       status as ref (Raw (thenPreds, elsePreds)))) =
		let
		    val trueSet2 = intersect (#1, thenPreds)
			(* set of `true' tests for `else' edge *)
		    val falseSet1 = intersect (#2, elsePreds)
			(* set of `false' tests for `then' edge *)
		    val trueSet1 = (pos, test)::trueSet2
			(* set of `true' tests for `then' edge *)
		    val falseSet2 = (pos, test)::falseSet1
			(* set of `false' tests for `else' edge *)
		    val sets = (trueSet1, falseSet2)
		in
		    status := Cooked sets;
		    getSets (!thenGraphRef); getSets (!elseGraphRef);
		    optimizeNode (thenGraphRef, trueSet1, falseSet1);
		    optimizeNode (elseGraphRef, trueSet2, falseSet2);
		    SOME sets
		end
	      | getSets (Node (_, _, _, _, ref (Cooked sets))) = SOME sets
	      | getSets (Leaf (_, _)) = NONE
	      | getSets _ = raise Crash.Crash "SimplifyMatch.getSets"
	    and intersect (sel, graph::graphs) =
		List.foldr (fn (graph, testSet) =>
			    testSetIntersect (sel (valOf (getSets graph)),
					      testSet))
		(sel (valOf (getSets graph))) graphs
	      | intersect (_, nil) = nil
	in
	    fun optimizeGraph graph =
		(computeRaw (graph, NONE, NONE); getSets graph; graph)
	end

	type consequent = Source.region * O.body option ref
	type mapping' = (pos * O.id) list

	fun buildGraph (matches, elseBody) =
	    let
		val (graph, consequents) =
		    Vector.foldr (fn ((region, pat, thenBody),
				      (elseTree, consequents)) =>
				  let
				      val pat' = separateAlt pat
				      val (testSeq, _) =
					  makeTestSeq (pat', nil, nil, nil)
				      val r = ref NONE
				      val leaf = Leaf (thenBody, r)
				  in
				      (mergeIntoTree (List.rev testSeq,
						      leaf, elseTree),
				       (region, r)::consequents)
				  end) (Default, nil) matches
		val elseGraph = Leaf (elseBody, ref NONE)
	    in
		case graph of
		    Default =>
			(elseGraph, consequents)
		  | _ =>
			(propagateElses (graph, elseGraph);
			 (optimizeGraph graph, consequents))
	    end

	(*
	 * Check whether the match rules of a function define
	 * a cartesian n-ary function; if they do, represent
	 * the cartesian arity explicitly.
	 *
	 * Preconditions:
	 * 1) No pattern binds the whole argument value to a variable.
	 * 2) No side effect can be performed by a GuardPat or WithPat
	 *    before the tuple or record is deconstructed (since in the
	 *    presence of by-need futures, the latter may also have
	 *    side effects).
	 *)

	local
	    exception MustBeUnary

	    exception BindsAll     (* precondition 1 not satisfied *)
	    exception SideEffect   (* precondition 2 not satisfied *)
	    exception NotNAry

	    fun deconstructs (JokPat _) = false
	      | deconstructs (LitPat _) = raise NotNAry
	      | deconstructs (VarPat (_, _)) = raise BindsAll
	      | deconstructs (TagPat (_, _, _, _)) = raise NotNAry
	      | deconstructs (ConPat (_, _, _, _)) = raise NotNAry
	      | deconstructs (RefPat _) = raise NotNAry
	      | deconstructs (TupPat (_, _)) = true
	      | deconstructs (ProdPat (_, _)) = true
	      | deconstructs (VecPat (_, _)) = raise NotNAry
	      | deconstructs (AsPat (_, pat1, pat2)) =
		deconstructs pat1 orelse deconstructs pat2
	      | deconstructs (AltPat (_, pats)) =
		Vector.exists deconstructs pats
	      | deconstructs (NegPat (_, pat)) = deconstructs pat
	      | deconstructs (GuardPat (_, pat, _)) =
		deconstructs pat orelse raise SideEffect
	      | deconstructs (WithPat (_, pat, _)) =
		deconstructs pat orelse raise SideEffect

	    fun checkMatches matches =
		(Vector.foldl (fn ((_, pat, _), b) =>
			       deconstructs pat orelse b) false matches)
		handle (BindsAll | SideEffect | NotNAry) => false

	    fun process (O.Unary, graph, consequents, info) =
		let
		    val id = O.freshId (id_info info)
		in
		    (O.OneArg (O.IdDef id), graph, [(nil, id)], consequents)
		end
	      | process (O.TupArity n, Node (nil, TupTest _, ref graph, _, _),
			 consequents, _) =
		let
		    val ids =
			Vector.tabulate
			(n, fn _ => O.freshId {region = Source.nowhere})
		    val mapping =
			Vector.foldri (fn (i, id, mapping) =>
				       ([LABEL (Label.fromInt (i + 1))], id)::
				       mapping) nil (ids, 0, NONE)
		    val idDefs = Vector.map O.IdDef ids
		in
		    (O.TupArgs idDefs, graph, mapping, consequents)
		end
	      | process (O.ProdArity labels,
			 Node (nil, ProdTest _, ref graph, _, _),
			 consequents, _) =
		let
		    val labelIdVec =
			Vector.map (fn label =>
				    (label,
				     O.freshId {region = Source.nowhere}))
			labels
		    val mapping =
			Vector.foldr (fn ((label, id), mapping) =>
				      ([LABEL label], id)::mapping)
			nil labelIdVec
		    val labelIdDefVec =
			Vector.map (fn (label, id) => (label, O.IdDef id))
			labelIdVec
		in
		    (O.ProdArgs labelIdDefVec, graph, mapping, consequents)
		end
	      | process (_, _, _, _) =
		raise Crash.Crash "SimplifyMatch.process"
	in
	    fun buildFunArgs (matches, errStms) =
		let
		    val info = infoPat (#2 (Vector.sub (matches, 0)))
		    val arity =
			if checkMatches matches then typToArity (#typ info)
			else O.Unary
		    val (graph, consequents) = buildGraph (matches, errStms)
		in
		    process (arity, graph, consequents, info)
		end
	end
    end
