module top (n0, n1, n2);
    input n0, n1;
    output n2;
    wire w0, w1, w2, n0, n1, n2;
    nand g0(w0, n1, n0);
    not g1(w1, w0);
    not g2(n2, w1);
endmodule