using System;

class Start {
    public static void Main(System.String[] args) {
	Alice.Komponist k = new Alice.Komponist();
	Alice.Komponist.global_k = k;
	
	if (args.Length < 1) {
	    Console.WriteLine("usage: start dll progargs");
	}
	else {
	    try {
		Alice.Builtins.Future_await.StaticApply(k.Import(args[0]));
	    }
	    catch (System.Reflection.TargetInvocationException e) {
		System.Exception ei = e.InnerException;

		if (ei is Alice.Values.Exception) {
		    Alice.Values.Exception ai = (Alice.Values.Exception) ei;
		    Console.Write("line ");
		    Console.Write(ai.Line);
		    Console.Write(": ");
		    Console.WriteLine(ai.Value);
		}
	    }
	}
    }
}
