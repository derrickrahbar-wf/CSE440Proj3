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
    b4b  : boolean;
    bb1 : AA;
END



class test
BEGIN   
VAR
    aa,bb,cc : integer;
    dd,ee : boolean;
    ff  : integer;
    testa : AA;
    testb : BB;
FUNCTION test;
BEGIN
    aa := 0;
    testa := new AA;
    testa.axl := 15;
    aa := testa.axl + 3;
    print testa.axl;
    print aa
END

END
.
