%%%
%%% Authors:
%%%   Thorsten Brunklaus <brunklaus@ps.uni-sb.de>
%%%
%%% Copyright:
%%%   Thorsten Brunklaus, 2000
%%%
%%% Last Change:
%%%   $Date$ by $Author$
%%%   $Revision$
%%%

functor
import
   GtkCoreComponent('GtkCore$' : GtkCore) at 'GtkCore'
   Native at 'GtkCanvas.so{native}'
export
   'Canvas$' : CANVAS
define
   %% Import necessary Core Definitions
   PointerToObject   = GtkCore.pointerToObject
   ObjectToPointer   = GtkCore.objectToPointer
   VaArgListToOzList = GtkCore.vaArgListToOzList

   %% Insert autogenerated Function Wrapper
   \insert 'CanvasWrapper.oz'

   %% Insert autogenerated Functor Interface
   \insert 'CanvasExport.oz'
end
