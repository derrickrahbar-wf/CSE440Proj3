(*testmd-10 insertion sort on array*)

(*testmd-11 some operations of array of classes *)

(* The following sorts a list of ArrayClasses based on their val *)

program test;

class ArrayClass
BEGIN
    VAR
        val : integer;
FUNCTION happy(aap, iip, cc : integer ; bb : boolean) : integer;
VAR
    aa : ARRAY [0..19] of ArrayClass;
    ii, jj, tmp : integer;
    tmpC : ArrayClass;
BEGIN
    ii := iip;
    print aap;
    print iip;
    print cc;
    print bb 
END

END

class test
BEGIN   
VAR
    aa : ARRAY [0..19] of ArrayClass;
    ii, jj, tmp : integer;
    bool : boolean;
    tmpC : ArrayClass;
FUNCTION test;
BEGIN
    tmpC := new ArrayClass;
    aa[1] := new ArrayClass;
    ii:= 17;
    aa[6].val := 34;
    bool := True;
    ii := tmpC.happy(5, aa[6].val, 2, bool)
    
END


END
.
