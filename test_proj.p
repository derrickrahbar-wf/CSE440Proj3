(* shows use of class attributes in between calles of same object *)

program test;

class AttrArray 
BEGIN
    VAR
    aa : ARRAY [0..19] of ArrayClass;
END

class ArrayClass
BEGIN
    VAR
        val : integer;

FUNCTION happy(VAR aap, iip: integer ; cc : integer ; bb : boolean) : integer;
VAR
    aa : ARRAY [0..19] of ArrayClass;
    ii, jj, tmp : integer;
    tmpC : ArrayClass;
BEGIN
    ii := iip;
    print True;
    print val; (* will be val set from arraysort *)
    print True;
    val := 250;
    aap := 233;
    iip := 343; (*shows will update actual var for pass by VAR*)

    happy := cc

END;

FUNCTION arraysort(VAR uns : AttrArray ; VAR size : integer) : AttrArray;
VAR 
        ii, tmp, jj, hold : integer;
        tmpC : ArrayClass;
        bool : boolean;

BEGIN
        ii := 1;
        val := 15;
        print this.val;

        print False;
        ii := this.happy(ii,jj,hold,bool);

        print this.val; (* will be different set in happy *)
        
        arraysort := uns

END

END


class test
BEGIN   
VAR
    aa : ARRAY [0..19] of ArrayClass;
    ii, jj, tmp : integer;
    bool : boolean;
    tmpC : ArrayClass;
    attr, uns : AttrArray;
FUNCTION test;
BEGIN
    attr := new AttrArray;
    ii := 0;
    while ii < 20 DO
    BEGIN
        attr.aa[ii]:= new ArrayClass;
        ii := ii + 1
    END;



    (*uns := tmpC.arraysort(attr, tmp)*)(*attr will be changed, uns will hold attr prev
                                            vals sorted *)


    ii := fib(0);
    print ii
      

END;

FUNCTION fib(xx :integer): integer;
VAR
    return : integer;
BEGIN
    

    if xx < 0 THEN
        return := -1
    ELSE if xx = 0 THEN
        return := 0
    ELSE if xx = 1 THEN
        return := 1
    ELSE 
        return := fib(xx-1) + fib(xx-2);

    fib := return
END


END
.
