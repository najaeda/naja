module adder(a, b, s); 
input  [7:0] a;
input  [7:0] b;
output   s;  

assign s = a || b;    

endmodule