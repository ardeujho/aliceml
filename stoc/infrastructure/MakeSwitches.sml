functor MakeSwitches(val logOut : TextIO.outstream) :> SWITCHES =
struct

    structure Warn =
    struct
	val shadowing				= ref false
    end

    structure Bootstrap =
    struct
	datatype rtt_level = NO_RTT | CORE_RTT | FULL_RTT

	val implicitImport			= ref true
	val implicitImportFile			= ref(NONE : string option)
	val rttLevel				= ref NO_RTT
    end

    structure Debug =
    struct
	val logOut				= logOut
	val logWidth				= ref 79

	val dumpPhases				= ref true
	val dumpAbstractionResult		= ref false
	val dumpElaborationResult		= ref false
	val dumpElaborationSig			= ref true
	val dumpIntermediate			= ref false
	val checkIntermediate			= ref false
	val dumpFlatteningResult		= ref false
	val dumpValuePropagationContext		= ref false
	val dumpValuePropagationResult		= ref false
	val dumpLivenessAnalysisIntermediate	= ref false
	val dumpLivenessAnalysisContext		= ref false
	val dumpLivenessAnalysisResult		= ref false
	val dumpDeadCodeEliminationResult	= ref false
	val dumpTarget				= ref false
    end

end
