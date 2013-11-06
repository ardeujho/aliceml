%%%
%%% Author:
%%%   Leif Kornstaedt <kornstae@ps.uni-sb.de>
%%%
%%% Contributor:
%%%   Andreas Rossberg <rossberg@ps.uni-sb.de>
%%%
%%% Copyright:
%%%   Leif Kornstaedt and Andreas Rossberg, 1999-2004
%%%
%%% Last change:
%%%   $Date$ by $Author$
%%%   $Revision$
%%%

\ifdef Mozart_1_2
\define OLD_BYNEED
\endif

functor
require
   BootName(newUnique: NewUniqueName '<' hash toString) at 'x-oz://boot/Name'
   BootFloat(fPow) at 'x-oz://boot/Float'
   BootWord at 'x-oz://boot/Word'
   BootAlice(rpc) at 'x-oz://boot/Alice'
export
   BuiltinTable
   RaiseAliceException
   UnwrapAliceException
prepare
   proc {RaiseAliceException E Coord}
      {Wait E}
      {Exception.raiseError alice(E Coord)}
   end

   fun {UnwrapAliceException E}
      case E of error(alice(InnerE ...) ...) then InnerE
      else {Exception.'raise' E} unit
      end
   end

   fun {DropDotReverse Cs Cs2}
      case Cs
      of nil then Cs2
      [] &.|_ then Cs2
      [] C|Rest then {DropDotReverse Rest C|Cs2}
      end
   end

   fun {NumberCompare I J}
      if I == J then 'EQUAL'
      elseif I < J then 'LESS'
      else 'GREATER'
      end
   end

   local
      fun {StringCompareSub S1 S2 L1 L2 N}
	 if N == L1 then
	    if L1 == L2 then 'EQUAL'
	    else 'LESS'
	    end
	 elseif N == L2 then 'GREATER'
	 elsecase {NumberCompare {ByteString.get S1 N} {ByteString.get S2 N}}
	 of 'EQUAL' then {StringCompareSub S1 S2 L1 L2 N + 1}
	 elseof X then X
	 end
      end
   in
      fun {StringCompare S1 S2}
	 {StringCompareSub S1 S2
	  {ByteString.length S1} {ByteString.length S2} 0}
      end
   end

   local
      W = 32
      W4 = {BootWord.make W 4}
      W24 = {BootWord.make W 24}
      FourHOBits = {BootWord.'<<' {BootWord.make W 0xF0} W24}
   in
      fun {StringHash S}
	 N = {ByteString.length S}
	 fun {Iter I H}
	    if I == N then H
	    else
	       C = {ByteString.get S I}
	       H2 = {BootWord.'+' {BootWord.'<<' H W4} {BootWord.make W C}}
	       G = {BootWord.andb H2 FourHOBits}
	       H3 = {BootWord.xorb {BootWord.xorb H2 {BootWord.'>>' G W24}} G}
	    in
	       {Iter I + 1 H3}
	    end
	 end
      in
	 {BootWord.toInt {Iter 0 {BootWord.make W 0}}}
      end
   end

   BuiltinTable =
   builtinTable(
      'op=':
	 fun {$ X Y}
	    {Wait X} % approximates proper behaviour for Alice equality
	    {Wait Y} % (nested futures may still compare prematurely)
	    X == Y
	 end
      'op<>':
	 fun {$ X Y}
	    {Wait X} % approximates proper behaviour for Alice inequality
	    {Wait Y} % (nested futures may still compare prematurely)
	    X \= Y
	 end
      'Array.array':
	 fun {$ N Init}
	    if 0 =< N andthen N < BuiltinTable.'Array.maxLen' then
	       {Array.new 0 N - 1 Init}
	    else
	       {Exception.raiseError alice(BuiltinTable.'General.Size')}
	       unit
	    end
	 end
      'Array.extract':
	 fun {$ A I N} V in
	    if 0 =< I andthen 0 =< N andthen I + N =< {Array.high A} + 1 then
	       V = {Tuple.make '#[]' N}
	       {Record.forAllInd V fun {$ J} A.(I + J - 1) end}
	       V
	    else
	       {Exception.raiseError alice(BuiltinTable.'General.Subscript')}
	       unit
	    end
	 end
      'Array.fromList':
	 fun {$ Xs} N A in
	    N = {Length Xs}
	    A = {Array.new 0 N - 1 unit}
	    {List.forAllInd Xs proc {$ I X} {Array.put A I - 1 X} end}
	    A
	 end
      'Array.fromVector':
	 fun {$ V} A in
	    A = {Array.new 0 {Width V} - 1 unit}
	    {For 1 {Width V} 1 proc {$ I} {Array.put A I - 1 V.I} end}
	    A
	 end
      'Array.length':
	 fun {$ A} {Array.high A} + 1 end
      'Array.maxLen': 0x7FFFFFF
      'Array.sub':
	 fun {$ A I}
	    try
	       {Array.get A I}
	    catch error(kernel(array ...) ...) then
	       {Exception.raiseError alice(BuiltinTable.'General.Subscript')}
	       unit
	    end
	 end
      'Array.tabulate':
	 fun {$ N F} A in
	    try
	       A = {Array.new 0 N - 1 unit}
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Size')}
	    end
	    {For 0 N - 1 1 proc {$ I} A.I := {F I} end}
	    A
	 end
      'Array.toList':
	 fun {$ A}
	    {ForThread {Array.high A} 0 ~1 fun {$ Xs I} A.I|Xs end nil}
	 end
      'Array.update':
	 fun {$ A I X}
	    try
	       {Array.put A I X}
	    catch error(kernel(array ...) ...) then
	       {Exception.raiseError alice(BuiltinTable.'General.Subscript')}
	    end
	    unit
	 end
      'Byte.bytesToString':
	 fun {$ S} S end
      'Byte.stringToBytes':
	 fun {$ S} S end
      'Char.<': Value.'<'
      'Char.>': Value.'>'
      'Char.<=': Value.'=<'
      'Char.>=': Value.'>='
      'Char.ord':
	 fun {$ C} C end
      'Char.chr':
	 fun {$ C}
	    if {Char.is C} then C
	    else
	       {Exception.raiseError alice(BuiltinTable.'General.Chr')}
	       unit
	    end
	 end
      'Char.isAlpha': Char.isAlpha
      'Char.isAlphaNum': Char.isAlNum
      'Char.isCntrl': Char.isCntrl
      'Char.isDigit': Char.isDigit
      'Char.isGraph': Char.isGraph
      'Char.isHexDigit': Char.isXDigit
      'Char.isLower': Char.isLower
      'Char.isPrint': Char.isPrint
      'Char.isPunct': Char.isPunct
      'Char.isSpace': Char.isSpace
      'Char.isUpper': Char.isUpper
      'Char.toLower': Char.toLower
      'Char.toUpper': Char.toUpper
      'CharArray.array':
	 fun {$ N Init}
	    if 0 =< N andthen N < BuiltinTable.'CharArray.maxLen' then
	       {Array.new 0 N - 1 Init}
	    else
	       {Exception.raiseError alice(BuiltinTable.'General.Size')}
	       unit
	    end
	 end
      'CharArray.extract':
	 fun {$ A I N}
	    if 0 =< I andthen 0 =< N andthen I + N =< {Array.high A} + 1 then
	       {ByteString.make {ForThread I + N - 1 I ~1
	        		 fun {$ Xs I} A.I|Xs end nil}}
	    else
	       {Exception.raiseError alice(BuiltinTable.'General.Subscript')}
	       unit
	    end
	 end
      'CharArray.fromList':
	 fun {$ Xs} N A in
	    N = {Length Xs}
	    A = {Array.new 0 N - 1 unit}
	    {List.forAllInd Xs proc {$ I X} {Array.put A I - 1 X} end}
	    A
	 end
      'CharArray.fromVector':
	 fun {$ V} A in
	    A = {Array.new 0 {ByteString.width V} - 1 unit}
	    {For 0 {ByteString.width V} 1
	     proc {$ I} {Array.put A I {ByteString.get V I}} end}
	    A
	 end
      'CharArray.length':
	 fun {$ A} {Array.high A} + 1 end
      'CharArray.maxLen': 0x7FFFFFF
      'CharArray.sub':
	 fun {$ A I}
	    try
	       {Array.get A I}
	    catch error(kernel(array ...) ...) then
	       {Exception.raiseError alice(BuiltinTable.'General.Subscript')}
	       unit
	    end
	 end
      'CharArray.tabulate':
	 fun {$ N F} A in
	    try
	       A = {Array.new 0 N - 1 unit}
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Size')}
	    end
	    {For 0 N - 1 1 proc {$ I} A.I := {F I} end}
	    A
	 end
      'CharArray.toList':
	 fun {$ A}
	    {ForThread {Array.high A} 0 ~1 fun {$ Xs I} A.I|Xs end nil}
	 end
      'CharArray.update':
	 fun {$ A I X}
	    try
	       {Array.put A I X}
	    catch error(kernel(array ...) ...) then
	       {Exception.raiseError alice(BuiltinTable.'General.Subscript')}
	    end
	    unit
	 end
      'CharVector.concat':
	 fun {$ Ss} {ByteString.make {List.toTuple '#' Ss}} end
      'CharVector.extract':
	 fun {$ V I N}
	    try
	       {ByteString.slice V I I + N}
	    catch system(kernel('ByteString.slice' ...) ...) then
	       {Exception.raiseError alice(BuiltinTable.'General.Subscript')}
	       unit
	    end
	 end
      'CharVector.fromList': ByteString.make
      'CharVector.maxLen': 0x1FFFFFFF
      'CharVector.length': ByteString.length
      'CharVector.sub':
	 fun {$ S I}
	    try
	       {ByteString.get S I}
	    catch system(kernel('ByteString.get' ...) ...) then
	       {Exception.raiseError alice(BuiltinTable.'General.Subscript')}
	       unit
	    end
	 end
      'CharVector.tabulate':
	 fun {$ N F} V in
	    try
	       V = {Tuple.make '#' N}
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Size')}
	    end
	    {For 1 N 1 proc {$ I} V.I = [{F I - 1}] end}
	    {ByteString.make V}
	 end
      'CharVector.toList':
	 fun {$ V}
	    {ForThread {ByteString.width V} - 1 0 ~1
	     fun {$ Xs I} {ByteString.get V I}|Xs end nil}
	 end
      'Exn.catch':
	 fun {$ Handler F}
	    try
	       {F unit}
	    catch error(alice(Exn ...) debug:Debug) then
	       {Handler E Debug}
	    end
	 end
      'Exn.dumpTrace':
	 fun {$ _ _}
	    unit %--**
	 end
      'Exn.name':
	 fun {$ N}
	    case {VirtualString.toString {Value.toVirtualString {Label N} 0 0}}
	    of "<N>" then {ByteString.make "_unknown"}
	    elseof &<|&N|&:|& |&'|Rest then
	       case {Reverse Rest} of &>|Rest then
		  {ByteString.make {DropDotReverse Rest nil}}
	       end
	    elseof &<|&N|&:|& |Rest then
	       case {Reverse Rest} of &>|Rest then
		  {ByteString.make {DropDotReverse Rest nil}}
	       end
	    elseof S then {ByteString.make S}
	    end
	 end
      'Exn.reraise':
	 fun {$ Exn Debug}
	    {Exception.'raise' error(alice(Exn) debug:Debug)}
	    unit
	 end
      'Future.Cyclic': {NewUniqueName 'Future.Cyclic'}
      'Future.alarm\'':
\ifdef OLD_BYNEED
	 fun {$ X} !!{Alarm X + 500} end
\else
	 fun {$ X} Y in
	    Y = !!{Alarm X + 500}
	    {Value.makeNeeded Y}
	    Y
	 end
\endif
      'Future.await':
	 fun {$ X} {Wait X} X end
      'Future.awaitEither\'':
	 fun {$ X Y} {WaitOr X Y} {Not {IsDet X}} end
      'Future.byneed':
\ifdef OLD_BYNEED
	 fun lazy {$ P}
	    try
	       {P unit}
	    catch error(InnerE ...) then
	       {Value.byNeedFail error(InnerE)}
	    end
	 end
\else
	 fun {$ P}
	    {ByNeedFuture fun {$} {P unit} end}
	 end
\endif
      'Future.concur':
\ifdef OLD_BYNEED
	 fun {$ P}
	    !!thread
		 try
		    {P unit}
		 catch error(InnerE ...) then
		    {Value.byNeedFail error(InnerE)}
		 end
	      end
	 end
\else
	 fun {$ P} Y in
	    Y = !!thread
		     try
			{P unit}
		     catch error(InnerE ...) then
			{Value.failed error(InnerE)}
		     end
		  end
	    {Value.makeNeeded Y}
	    Y
	 end
\endif
      'Future.isByneed':
\ifdef OLD_BYNEED
	 fun {$ X}
	    {IsFuture X} andthen
	    case {Value.toVirtualString X 0 0} of
	       &_|&<|&f|&u|&t|&u|&r|&e|& |&b|&y|&N|&e|&e|&d|_ then true
	    else false
	    end
	 end
\else
	 fun {$ X}
	    {IsFuture X} andthen {Not {IsNeeded X}}
	 end
\endif
      'Future.status':
\ifdef OLD_BYNEED
         fun {$ X}
            if {IsFuture X} then
               if {Value.isFailed X} then 'FAILED'
               else 'FUTURE'
               end
            else 'DETERMINED'
            end
         end
\else
         fun {$ X}
            case {Value.status X}
            of future then 'FUTURE'
            [] failed then 'FAILED'
            else 'DETERMINED'
            end
         end
\endif
      'General.Assert': {NewUniqueName 'General.Assert'}
      'General.Bind': {NewUniqueName 'General.Bind'}
      'General.Chr': {NewUniqueName 'General.Chr'}
      'General.Div': {NewUniqueName 'General.Div'}
      'General.Domain': {NewUniqueName 'General.Domain'}
      'General.Fail': {NewUniqueName 'General.Fail'}
      'General.Match': {NewUniqueName 'General.Match'}
      'General.Overflow': {NewUniqueName 'General.Overflow'}
      'General.Size': {NewUniqueName 'General.Size'}
      'General.Span': {NewUniqueName 'General.Span'}
      'General.Subscript': {NewUniqueName 'General.Subscript'}
      'GlobalStamp.new':
	 fun {$ unit} {NewName} end
      'GlobalStamp.fromString':
	 fun {$ S} {NewUniqueName {VirtualString.toAtom S}} end
      'GlobalStamp.toString':
	 fun {$ N}
	    case {Value.toVirtualString N 0 0}
	    of &<|&N|&:|& |Rest then
	       case {Reverse Rest} of &>|Rest2 then
		  {ByteString.make {Reverse Rest2}}
	       end
	    elseof S then {ByteString.make S}
	    end
	 end
	 % fun {$ N} {ByteString.make {BootName.toString N}} end
      'GlobalStamp.compare':
	 fun {$ N1 N2}
	    if N1 == N2 then 'EQUAL'
	    elseif {BootName.'<' N1 N2} then 'LESS'
	    else 'GREATER'
	    end
	 end
      'GlobalStamp.hash': BootName.hash
      'Hole.Hole': {NewUniqueName 'Hole.Hole'}
      'Hole.fail':
	 fun {$ X E}
	    {Wait E}
	    try
	       X = {Value.byNeedFail error(alice(E))}
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'Hole.Hole')}
	    end
	    unit
	 end
      'Hole.fill':
	 fun {$ X Y}
	    if {IsDet X} then   %--** test and bind must be atomic
	       {Exception.raiseError alice(BuiltinTable.'Hole.Hole')}
	    end
	    try
	       X = Y
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'Hole.Hole')}
	    end
	    unit
	 end
      'Hole.future':
	 fun {$ X} Y in
	    %%if {IsFuture X} orelse {Not {IsFree X}} then
	    %%   {Exception.raiseError alice(BuiltinTable.'Hole.Hole')}
	    %%end
	    Y = !!X
\ifndef OLD_BYNEED
	    {Value.makeNeeded Y}
\endif
	    Y
	 end
      'Hole.hole':
	 fun {$ unit} _ end
      'Hole.isHole': IsFree
      'IEEEReal.getRoundingMode': 
         fun {$ _}
            {Exception.raiseError
             alice(BuiltinTable.'General.Fail'
                   ({ByteString.make "Not implemented on Mozart: IEEEReal.getRoundingMode"}))}
            unit
         end         
      'IEEEReal.setRoundingMode':
         fun {$ _}
            {Exception.raiseError
             alice(BuiltinTable.'General.Fail'
                   ({ByteString.make "Not implemented Mozart: IEEEReal.setRoundingMode"}))}
            unit
         end         
      'Int.~': Number.'~'
      'Int.+': Number.'+'
      'Int.-': Number.'-'
      'Int.*': Number.'*'
      'Int.<': Value.'<'
      'Int.>': Value.'>'
      'Int.<=': Value.'=<'
      'Int.>=': Value.'>='
      'Int.abs': Abs
      'Int.compare': NumberCompare
      'Int.div':
	 fun {$ X1 X2}
	    try B1 B2 in
	       B1 = {Int.isNat X1}
	       B2 = {Int.isNat X2}
	       if B1 == B2 then
		  X1 div X2
	       elseif B2 then
		  (X1 - X2 + 1) div X2
	       else
		  (X1 - X2 - 1) div X2
	       end
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Div')}
	       unit
	    end
	 end
      'Int.maxInt': 'NONE'
      'Int.minInt': 'NONE'
      'Int.mod':
	 fun {$ X1 X2}
	    try A in
	       A = X1 mod X2
	       if A == 0 then A
	       elseif A < 0 then
		  if X2 =< 0 then A
		  else A + X2
		  end
	       else   % A > 0
		  if X2 < 0 then A + X2
		  else A
		  end
	       end
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Div')}
	       unit
	    end
	 end
      'Int.precision': 'NONE'
      'Int.quot':
	 fun {$ X1 X2}
	    try
	       X1 div X2
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Div')}
	       unit
	    end
	 end
      'Int.rem':
	 fun {$ X1 X2}
	    try
	       X1 mod X2
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Div')}
	       unit
	    end
	 end

      'IntInf.fromInt' : fun {$ X} X end
      'IntInf.toInt' : fun {$ X} X end
      'IntInf.~': Number.'~'
      'IntInf.+': Number.'+'
      'IntInf.-': Number.'-'
      'IntInf.*': Number.'*'
      'IntInf.<': Value.'<'
      'IntInf.>': Value.'>'
      'IntInf.<=': Value.'=<'
      'IntInf.>=': Value.'>='
      'IntInf.abs': Abs
      'IntInf.compare': NumberCompare
      'IntInf.div':
	 fun {$ X1 X2}
	    try B1 B2 in
	       B1 = {Int.isNat X1}
	       B2 = {Int.isNat X2}
	       if B1 == B2 then
		  X1 div X2
	       elseif B2 then
		  (X1 - X2 + 1) div X2
	       else
		  (X1 - X2 - 1) div X2
	       end
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Div')}
	       unit
	    end
	 end
      'IntInf.mod':
	 fun {$ X1 X2}
	    try A in
	       A = X1 mod X2
	       if A == 0 then A
	       elseif A < 0 then
		  if X2 =< 0 then A
		  else A + X2
		  end
	       else   % A > 0
		  if X2 < 0 then A + X2
		  else A
		  end
	       end
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Div')}
	       unit
	    end
	 end
      'IntInf.quot':
	 fun {$ X1 X2}
	    try
	       X1 div X2
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Div')}
	       unit
	    end
	 end
      'IntInf.rem':
	 fun {$ X1 X2}
	    try
	       X1 mod X2
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Div')}
	       unit
	    end
	 end
      'IntInf.divMod':
	 fun {$ X1 X2}
	    try A B1 B2 D M in
	       B1 = {Int.isNat X1}
	       B2 = {Int.isNat X2}
	       if B1 == B2 then
		  D = X1 div X2
	       elseif B2 then
		  D = (X1 - X2 + 1) div X2
	       else
		  D = (X1 - X2 - 1) div X2
	       end
	       A = X1 mod X2
	       if A == 0 then M=A
	       elseif A < 0 then
		  if X2 =< 0 then M=A
		  else M=A + X2
		  end
	       else   % A > 0
		  if X2 < 0 then M=A + X2
		  else M=A
		  end
	       end               
               D#M
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Div')}
	       unit
	    end
	 end
      'IntInf.quotRem':
	 fun {$ X1 X2}
	    try Q R in
	       Q = X1 div X2
               R = X1 mod X2
               Q#R
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Div')}
	       unit
	    end
	 end
      'IntInf.pow': Number.pow
      'IntInf.log2':
         fun {$ _ _}
            {Exception.raiseError
             alice(BuiltinTable.'General.Fail'
                   ({ByteString.make "Not implemented: IntInf.log2"}))}
            unit
         end
      'IntInf.orb':
         fun {$ _ _}
            {Exception.raiseError
             alice(BuiltinTable.'General.Fail'
                   ({ByteString.make "Not implemented: IntInf.orb"}))}
            unit
         end
      'IntInf.xorb':
         fun {$ _ _}
            {Exception.raiseError
             alice(BuiltinTable.'General.Fail'
                   ({ByteString.make "Not implemented: IntInf.xorb"}))}
            unit
         end
      'IntInf.andb':
         fun {$ _ _}
            {Exception.raiseError
             alice(BuiltinTable.'General.Fail'
                   ({ByteString.make "Not implemented: IntInf.andb"}))}
            unit
         end         
      'IntInf.notb':
         fun {$ _}
            {Exception.raiseError
             alice(BuiltinTable.'General.Fail'
                   ({ByteString.make "Not implemented: IntInf.notb"}))}
            unit
         end         
      'IntInf.<<':
         fun {$ _ _}
            {Exception.raiseError
             alice(BuiltinTable.'General.Fail'
                   ({ByteString.make "Not implemented: IntInf.<<"}))}
            unit
         end         
      'IntInf.~>>':
         fun {$ _ _}
            {Exception.raiseError
             alice(BuiltinTable.'General.Fail'
                   ({ByteString.make "Not implemented: IntInf.xorb"}))}
            unit
         end         

      'List.Empty': {NewUniqueName 'List.Empty'}
      'Math.acos': Acos
      'Math.acosh': Float.acosh
      'Math.asin': Asin
      'Math.asinh': Float.asinh
      'Math.atan': Atan
      'Math.atanh': Float.atanh
      'Math.atan2': Atan2
      'Math.cos': Cos
      'Math.cosh': Float.cosh
      'Math.e': 2.71828182846
      'Math.exp': Exp
      'Math.ln': Log
      'Math.pi': 3.14159265359
      'Math.pow': BootFloat.fPow
      'Math.sin': Sin
      'Math.sinh': Float.sinh
      'Math.sqrt': Sqrt
      'Math.tan': Tan
      'Math.tanh': Float.tanh
      'Option.Option': {NewUniqueName 'Option.Option'}
      'Real.~': Number.'~'
      'Real.+': Number.'+'
      'Real.-': Number.'-'
      'Real.*': Number.'*'
      'Real./': Float.'/'
      'Real.<': Value.'<'
      'Real.>': Value.'>'
      'Real.<=': Value.'=<'
      'Real.>=': Value.'>='
      'Real.ceil':
	 fun {$ R}
	    {FloatToInt {Ceil R}}
	 end
      'Real.largeCeil':
	 fun {$ R}
	    {FloatToInt {Ceil R}}
	 end
      'Real.compare': NumberCompare
      'Real.floor':
	 fun {$ R}
	    {FloatToInt {Floor R}}
	 end
      'Real.largeFloor':
	 fun {$ R}
	    {FloatToInt {Floor R}}
	 end
      'Real.fromInt': IntToFloat
      'Real.fromLargeInt': IntToFloat
      'Real.precision': 52
      'Real.realCeil': Ceil
      'Real.realFloor': Floor
      'Real.realRound': Round
      'Real.realTrunc':
	 fun {$ R}
	    if R >= 0.0 then {Floor R} else {Ceil R} end
	 end
      'Real.rem': Float.'mod'
      'Real.round':
	 fun {$ R}
	    {FloatToInt {Round R}}
	 end
      'Real.largeRound':
	 fun {$ R}
	    {FloatToInt {Round R}}
	 end
      'Real.toString':
	 fun {$ R}
	    {ByteString.make {FloatToString R}}
	 end
      'Real.trunc':
	 fun {$ R}
	    {FloatToInt if R >= 0.0 then {Floor R} else {Ceil R} end}
	 end
      'Real.largeTrunc':
	 fun {$ R}
	    {FloatToInt if R >= 0.0 then {Floor R} else {Ceil R} end}
	 end
      'Ref.:=':
	 fun {$ R X} {Assign R X} unit end
      'Ref.exchange':
	 fun {$ R X} {Exchange R $ X} end
      'Remote.dynamicCall':
	 fun {$ A B}
	    try {BootAlice.rpc A B}
	    catch alice(undefinedProperty) then %--** needs to be preregistered
	       {Exception.raiseError alice(BuiltinTable.'Hole.Hole')} unit
	    end
	 end
      'String.^':
	 fun {$ S1 S2} {ByteString.append S1 S2} end
      'String.<':
	 fun {$ S1 S2} {StringCompare S1 S2} == 'LESS' end
      'String.>':
	 fun {$ S1 S2} {StringCompare S1 S2} == 'GREATER' end
      'String.<=':
	 fun {$ S1 S2} {StringCompare S1 S2} \= 'GREATER' end
      'String.>=':
	 fun {$ S1 S2} {StringCompare S1 S2} \= 'LESS' end
      'String.compare': StringCompare
      'String.hash': StringHash
      'String.maxSize': 0x7FFFFFFF
      'String.str':
	 fun {$ C} {ByteString.make [C]} end
      'Thread.Terminate': kernel(terminate)
      'Thread.Terminated': {NewUniqueName 'Thread.Terminated'}
      'Thread.current':
	 fun {$ unit} {Thread.this} end
      'Thread.isSuspended': Thread.isSuspended
      'Thread.raiseIn':
	 fun {$ T E}
	    {Wait E}
	    try {Thread.injectException T error(alice(E) debug: unit)}
	    catch error(kernel(deadThread _) ...) then
	       {Exception.raiseError alice(BuiltinTable.'Thread.Terminated')}
	    end
	    unit
	 end
      'Thread.resume':
	 fun {$ T}
	    try {Thread.resume T}
	    catch error(kernel(deadThread _) ...) then skip
	    end
	    unit
	 end
      'Thread.state':
	 fun {$ T}
	    case {Thread.state T} of runnable then 'RUNNABLE'
	    [] blocked then 'BLOCKED'
	    [] terminated then 'TERMINATED'
	    end
	 end
      'Thread.suspend':
	 fun {$ T}
	    try {Thread.suspend T}
	    catch error(kernel(deadThread _) ...) then skip
	    end
	    unit
	 end
      'Thread.yield':
	 fun {$ T}
	    try {Thread.preempt T}
	    catch error(kernel(deadThread _) ...) then skip
	    end
	    unit
	 end
      'UniqueString.hash':
	 fun {$ A} {StringHash {ByteString.make A}} end
      'UniqueString.string':
	 fun {$ A} {ByteString.make {Atom.toString A}} end
      'UniqueString.unique': VirtualString.toAtom
      'Unsafe.Array.sub': Array.get
      'Unsafe.Array.update':
	 fun {$ A I X} {Array.put A I X} unit end
      'Unsafe.String.sub': ByteString.get
      'Unsafe.Vector.sub':
	 fun {$ V I} V.(I + 1) end
      'Unsafe.cast': fun {$ X} X end
      'Vector.concat':
	 fun {$ Vs} N V in
	    N = {List.foldR Vs fun {$ V In} {Width V} + In end 0}
	    V = {Tuple.make '#[]' N}
	    N = {List.foldL Vs
		 fun {$ In V0}
		    {Record.forAllInd V0 proc {$ I X} V.(In + I) = X end}
		    In + {Width V0}
		 end 0}
	    V
	 end
      'Vector.extract':
	 fun {$ V I N} V2 in
	    if 0 =< I andthen 0 =< N andthen I + N =< {Width V} then
	       V2 = {Tuple.make '#[]' N}
	       {Record.forAllInd V2 fun {$ J} V.(I + J) end}
	       V2
	    else
	       {Exception.raiseError alice(BuiltinTable.'General.Subscript')}
	       unit
	    end
	 end
      'Vector.fromList':
	 fun {$ Xs} {List.toTuple '#[]' Xs} end
      'Vector.maxLen': 0x7FFFFFF
      'Vector.length': Width
      'Vector.sub':
	 fun {$ V I}
	    try
	       V.(I + 1)
	    catch error(kernel('.' ...) ...) then
	       {Exception.raiseError alice(BuiltinTable.'General.Subscript')}
	       unit
	    end
	 end
      'Vector.tabulate':
	 fun {$ N F} V in
	    try
	       V = {Tuple.make '#[]' N}
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Size')}
	    end
	    {For 1 N 1 proc {$ I} V.I = {F I - 1} end}
	    V
	 end
      'Vector.toList':
	 fun {$ V}
	    {ForThread {Width V} 1 ~1 fun {$ Xs I} V.I|Xs end nil}
	 end
      'Word8.+': BootWord.'+'
      'Word8.-': BootWord.'-'
      'Word8.*': BootWord.'*'
      'Word8.<<': BootWord.'<<'
      'Word8.>>': BootWord.'>>'
      'Word8.~>>': BootWord.'~>>'
      'Word8.<': BootWord.'<'
      'Word8.>': BootWord.'>'
      'Word8.<=': BootWord.'=<'
      'Word8.>=': BootWord.'>='
      'Word8.andb': BootWord.'andb'
      'Word8.div':
	 fun {$ W1 W2}
	    try
	       {BootWord.'div' W1 W2}
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Div')}
	       unit
	    end
	 end
      'Word8.fromInt':
	 fun {$ I} {BootWord.make 8 I} end
      'Word8.fromLargeInt':
	 fun {$ I} {BootWord.make 8 I} end
      'Word8.fromLarge':
	 fun {$ W} {BootWord.make 8 {BootWord.toInt W}} end
      'Word8.fromLargeX':
	 fun {$ W} {BootWord.make 8 {BootWord.toIntX W}} end
      'Word8.mod':
	 fun {$ W1 W2}
	    try
	       {BootWord.'mod' W1 W2}
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Div')}
	       unit
	    end
	 end
      'Word8.notb': BootWord.notb
      'Word8.orb': BootWord.orb
      'Word8.toInt': BootWord.toInt
      'Word8.toIntX': BootWord.toIntX
      'Word8.toLargeInt': BootWord.toInt
      'Word8.toLargeIntX': BootWord.toIntX
      'Word8.toLarge':
	 fun {$ W} {BootWord.make 32 {BootWord.toInt W}} end
      'Word8.toLargeX':
	 fun {$ W} {BootWord.make 32 {BootWord.toIntX W}} end
      'Word8.wordSize': 31
      'Word8.xorb': BootWord.'xorb'
      'Word8Array.array':
	 fun {$ N Init}
	    if 0 =< N andthen N < BuiltinTable.'Word8Array.maxLen' then
	       {Array.new 0 N - 1 Init}
	    else
	       {Exception.raiseError alice(BuiltinTable.'General.Size')}
	       unit
	    end
	 end
      'Word8Array.extract':
	 fun {$ A I N}
	    if 0 =< I andthen 0 =< N andthen I + N =< {Array.high A} + 1 then
	       {ByteString.make {ForThread I + N - 1 I ~1
				 fun {$ Xs I} {BootWord.toInt A.I}|Xs end nil}}
	    else
	       {Exception.raiseError alice(BuiltinTable.'General.Subscript')}
	       unit
	    end
	 end
      'Word8Array.fromList':
	 fun {$ Xs} N A in
	    N = {Length Xs}
	    A = {Array.new 0 N - 1 unit}
	    {List.forAllInd Xs proc {$ I X} {Array.put A I - 1 X} end}
	    A
	 end
      'Word8Array.fromVector':
	 fun {$ V} A in
	    A = {Array.new 0 {ByteString.width V} - 1 unit}
	    {For 0 {ByteString.width V} 1
	     proc {$ I}
		{Array.put A I {BootWord.make 8 {ByteString.get V I}}}
	     end}
	    A
	 end
      'Word8Array.length':
	 fun {$ A} {Array.high A} + 1 end
      'Word8Array.maxLen': 0x7FFFFFF
      'Word8Array.sub':
	 fun {$ A I}
	    try
	       {Array.get A I}
	    catch error(kernel(array ...) ...) then
	       {Exception.raiseError alice(BuiltinTable.'General.Subscript')}
	       unit
	    end
	 end
      'Word8Array.tabulate':
	 fun {$ N F} A in
	    try
	       A = {Array.new 0 N - 1 unit}
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Size')}
	    end
	    {For 0 N - 1 1 proc {$ I} A.I := {F I} end}
	    A
	 end
      'Word8Array.toList':
	 fun {$ A}
	    {ForThread {Array.high A} 0 ~1
	     fun {$ Xs I} {BootWord.make 8 A.I}|Xs end nil}
	 end
      'Word8Array.update':
	 fun {$ A I X}
	    try
	       {Array.put A I X}
	    catch error(kernel(array ...) ...) then
	       {Exception.raiseError alice(BuiltinTable.'General.Subscript')}
	    end
	    unit
	 end
      'Word8Vector.concat':
	 fun {$ Ss} {ByteString.make {List.toTuple '#' Ss}} end
      'Word8Vector.extract':
	 fun {$ V I N}
	    try
	       {ByteString.slice V I I + N}
	    catch system(kernel('ByteString.slice' ...) ...) then
	       {Exception.raiseError alice(BuiltinTable.'General.Subscript')}
	       unit
	    end
	 end
      'Word8Vector.fromList':
	 fun {$ Xs} {ByteString.make {List.map Xs BootWord.toInt}} end
      'Word8Vector.maxLen': 0x1FFFFFFF
      'Word8Vector.length': ByteString.length
      'Word8Vector.sub':
	 fun {$ S I}
	    try
	       {BootWord.make 8 {ByteString.get S I}}
	    catch system(kernel('ByteString.get' ...) ...) then
	       {Exception.raiseError alice(BuiltinTable.'General.Subscript')}
	       unit
	    end
	 end
      'Word8Vector.tabulate':
	 fun {$ N F} V in
	    try
	       V = {Tuple.make '#' N}
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Size')}
	    end
	    {For 1 N 1 proc {$ I} V.I = [{BootWord.toInt {F I - 1}}] end}
	    {ByteString.make V}
	 end
      'Word8Vector.toList':
	 fun {$ V}
	    {ForThread {ByteString.width V} - 1 0 ~1
	     fun {$ Xs I} {BootWord.make 8 {ByteString.get V I}}|Xs end nil}
	 end
      'Word31.+': BootWord.'+'
      'Word31.-': BootWord.'-'
      'Word31.*': BootWord.'*'
      'Word31.<<': BootWord.'<<'
      'Word31.>>': BootWord.'>>'
      'Word31.~>>': BootWord.'~>>'
      'Word31.<': BootWord.'<'
      'Word31.>': BootWord.'>'
      'Word31.<=': BootWord.'=<'
      'Word31.>=': BootWord.'>='
      'Word31.andb': BootWord.'andb'
      'Word31.div':
	 fun {$ W1 W2}
	    try
	       {BootWord.'div' W1 W2}
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Div')}
	       unit
	    end
	 end
      'Word31.fromInt':
	 fun {$ I} {BootWord.make 31 I} end
      'Word31.fromLargeInt':
	 fun {$ I} {BootWord.make 31 I} end
      'Word31.fromLarge':
	 fun {$ W} {BootWord.make 31 {BootWord.toInt W}} end
      'Word31.fromLargeX':
	 fun {$ W} {BootWord.make 31 {BootWord.toIntX W}} end
      'Word31.mod':
	 fun {$ W1 W2}
	    try
	       {BootWord.'mod' W1 W2}
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Div')}
	       unit
	    end
	 end
      'Word31.notb': BootWord.notb
      'Word31.orb': BootWord.orb
      'Word31.toInt': BootWord.toInt
      'Word31.toIntX': BootWord.toIntX
      'Word31.toLargeInt': BootWord.toInt
      'Word31.toLargeIntX': BootWord.toIntX
      'Word31.toLarge':
	 fun {$ W} {BootWord.make 32 {BootWord.toInt W}} end
      'Word31.toLargeX':
	 fun {$ W} {BootWord.make 32 {BootWord.toIntX W}} end
      'Word31.wordSize': 31
      'Word31.xorb': BootWord.'xorb'
      'Word32.+': BootWord.'+'
      'Word32.-': BootWord.'-'
      'Word32.*': BootWord.'*'
      'Word32.<<': BootWord.'<<'
      'Word32.>>': BootWord.'>>'
      'Word32.~>>': BootWord.'~>>'
      'Word32.<': BootWord.'<'
      'Word32.>': BootWord.'>'
      'Word32.<=': BootWord.'=<'
      'Word32.>=': BootWord.'>='
      'Word32.andb': BootWord.'andb'
      'Word32.div':
	 fun {$ W1 W2}
	    try
	       {BootWord.'div' W1 W2}
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Div')}
	       unit
	    end
	 end
      'Word32.fromInt':
	 fun {$ I} {BootWord.make 32 I} end
      'Word32.fromLargeInt':
	 fun {$ I} {BootWord.make 32 I} end
      'Word32.fromLarge':
	 fun {$ W} W end
      'Word32.fromLargeX':
	 fun {$ W} {BootWord.make 32 {BootWord.toIntX W}} end
      'Word32.mod':
	 fun {$ W1 W2}
	    try
	       {BootWord.'mod' W1 W2}
	    catch _ then
	       {Exception.raiseError alice(BuiltinTable.'General.Div')}
	       unit
	    end
	 end
      'Word32.notb': BootWord.notb
      'Word32.orb': BootWord.orb
      'Word32.toInt': BootWord.toInt
      'Word32.toIntX': BootWord.toIntX
      'Word32.toLargeInt': BootWord.toInt
      'Word32.toLargeIntX': BootWord.toIntX
      'Word32.toLarge':
	 fun {$ W} W end
      'Word32.toLargeX':
	 fun {$ W} {BootWord.make 32 {BootWord.toIntX W}} end
      'Word32.wordSize': 32
      'Word32.xorb': BootWord.'xorb')
end
