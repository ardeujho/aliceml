(*****************************************************************************
 * Word8Array
 *****************************************************************************)

structure Word8Array =
struct
    open Word8Array

    fun copy{src, dst, di} =
	Word8Array.copy{src=src, dst=dst, di=di, si=0, len=NONE}
end


(*****************************************************************************
 * Word8ArraySlice
 *****************************************************************************)

structure Word8ArraySlice =
struct
    val slice = Word8Array.extract	(* dummies! *)
    fun vector sl = sl
end


(*****************************************************************************
 * TextIO
 *****************************************************************************)

structure TextIO =
  struct
    open TextIO
    val inputLine = fn f =>
	case inputLine f
	  of "" => NONE
	   | s  => SOME s
  end



(*****************************************************************************
 * OS
 *****************************************************************************)

structure OS =
  struct
    open OS
    structure Path =
      struct
	open Path
	val mkAbsolute = fn{path, relativeTo} => mkAbsolute(path, relativeTo)
	val mkRelative = fn{path, relativeTo} => mkRelative(path, relativeTo)
      end
    structure FileSys =
      struct
        open FileSys
	fun getDir() =
	    let
		val p = FileSys.getDir()
	    in
		if String.size p > 1 andalso String.sub(p,1) = #":" then
		    String.str(Char.toLower(String.sub(p,0))) ^
		    String.extract(p, 1, NONE)
		else
		    p
	    end
      end
  end

