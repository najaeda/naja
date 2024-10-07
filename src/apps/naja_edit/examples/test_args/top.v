module top(input a, input b, output c);
  wire w;
  
  module1 m1(a, b, w);
  module2 m2(w, c);
endmodule