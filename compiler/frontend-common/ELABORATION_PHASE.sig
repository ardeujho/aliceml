signature ELABORATION_PHASE =
  sig

    structure C : CONTEXT          = Env
    structure I : ABSTRACT_GRAMMAR = AbstractGrammar
    structure O : ABSTRACT_GRAMMAR = TypedGrammar

    val translate : Env.t -> I.comp -> O.comp

  end
