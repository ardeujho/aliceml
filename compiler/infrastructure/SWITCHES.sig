signature SWITCHES =
  sig

    structure Warn :
    sig
	val shadowing :				bool ref
    end

    structure Bootstrap :
    sig
	datatype rtt_level = NO_RTT | CORE_RTT | FULL_RTT

	val implicitImport :			bool ref
	val implicitImportFile :		string option ref
	val rttLevel :				rtt_level ref
    end

    structure Debug :
    sig
	val logOut :				TextIO.outstream
	val logWidth :				int ref

	val dumpPhases :			bool ref
	val dumpAbstractionResult :		bool ref
	val dumpElaborationResult :		bool ref
	val dumpElaborationSig :		bool ref
	val dumpIntermediate :			bool ref
	val checkIntermediate :			bool ref
	val dumpFlatteningResult :		bool ref
	val dumpValuePropagationContext :	bool ref
	val dumpValuePropagationResult :	bool ref
	val dumpLivenessAnalysisIntermediate :	bool ref
	val dumpLivenessAnalysisContext :	bool ref
	val dumpLivenessAnalysisResult :	bool ref
	val dumpDeadCodeEliminationResult :	bool ref
	val dumpTarget :			bool ref
    end

  end
