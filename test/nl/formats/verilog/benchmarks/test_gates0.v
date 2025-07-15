/*
  test_gates0: Test "generic RTL" code.
*/

module FA(A, B, C, OUT, COUT);
   input A;
   input B, C;
   output OUT, COUT;

   //line[0] = C & A
   and and0 (line[0], C, A);

   //line[2] = ~(C ^ A)
   xor xor1 (line[1], C, A);
   not not2 (line[2], line[1]);

   //line[4] = B & ~(line[2])
   not not3 (line[3], line[2]);
   and and4 (line[4], line[3], B);

   //COUT = line[4] | line[0]
   or or5 (COUT, line[4], line[0]);

   //OUT = ~(line[2] ^ B)
   xor xor6 (line[5], line[2], B);
   not not7 (line[6], line[5]);

   buf buf8 (OUT, line[6]);
   
   //assign {COUT, OUT} = A+B+C;
endmodule