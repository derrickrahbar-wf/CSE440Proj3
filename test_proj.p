program test;




class AA
BEGIN   
VAR
    axl,bxl : integer;
    gfd,exe : boolean;
    ljs  : integer;
END



class BB extends AA
BEGIN   
VAR
    bb1,bb2,bb3 : integer;
    b4b  : boolean;
END



class tes
BEGIN   
VAR
    aa,bb,cc : integer;
    dd,ee : boolean;
    ff  : integer;
    testa : AA;
    testb : BB;
FUNCTION test;
BEGIN
    aa := 0
END

END
.
