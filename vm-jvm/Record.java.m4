package de.uni_sb.ps.DML.DMLRuntime;

final public class DMLRecord implements DMLValue {

    static private java.util.Hashtable arityHash = new java.util.Hashtable();

    public DMLRecord (DMLLabel[] ls, DMLValue[] vals) {
	Object ar=null;
	this.vals=vals;
	arity=new DMLRecordArity(ls, vals);
	ar=arityHash.get(arity);
	if (ar==null)
	    arityHash.put(arity,arity);
	else
	    arity = (DMLRecordArity) ar;
    }

    /** funktioniert nur, wenn records unique sind. */
    final public boolean equals(Object val) {
	DMLRecord r=null;
	int i=0;
	if (!(val instanceof DMLRecord))
	    return false;
	else {
	    r = (DMLRecord) val;
	    if (!r.getRecordArity().equals(this.arity))
		return false;
	    for(i=0; i<vals.length; i++)
		if (!vals[i].equals(r.vals[i])) return false;
	    return true;
	}
    }

    DMLValue vals[]=null;

    final public String toString() {
	String s="{";
	int i;
	for (i=0; i<vals.length;i++) {
	    if (i>0) s+=", ";
	    s+=arity.getLabel(i)+" = "+vals[i];
	}
	return s+"}";
    }

    final public DMLValue getByLabel(int i) {
	int index = arity.getIndexOfLabel(new DMLLabel(i));
	if (index > -1)
	    return vals[index];
	else
	    return null;
    }

    final public DMLValue getByLabel(String s) {
	int index = arity.getIndexOfLabel(new DMLLabel(s));
	if (index > -1)
	    return vals[index];
	else
	    return null;
    }

    final public DMLValue getByLabel(DMLLabel label) {
	int index = arity.getIndexOfLabel(label);
	if (index > -1)
	    return vals[index];
	else
	    return null;
    }

    DMLRecordArity arity=null;

    /** gibt den i-ten Eintrag des Records */
    final public DMLValue getByIndex(int i){
	return vals[i];
    }

    /** gibt die Stelligkeit des Records an */
    final public int getArity() {
	return vals.length;
    }

    /** @parameter: arity,
	@returns: null, falls es diese Arity noch nicht gab,
	Zeiger auf die bereits dagewesene Arity sonst */
    final static public DMLRecordArity getRecordArity(DMLRecordArity arity) {
	return  (DMLRecordArity) arityHash.get(arity);
    }

    final public DMLRecordArity getRecordArity() {
	return arity;
    }

    final public DMLValue getValue() {
	return this;
    }

    final public DMLValue request() {
	return this;
    }

    final public DMLValue apply(DMLValue v) {
	return DMLConstants.runtimeError.apply(new DMLString("cannot apply "+this+" to "+v)).raise();
    }

    final public DMLValue raise() {
	throw new DMLExceptionWrapper(this);
    }
}
