module halfadder(
    input a,
    input b,
    output sum,
    output carry
);
    and carry_and(carry, a, b);
    xor sum_xor(sum, a, b);
endmodule

module fulladder(
    input a,
    input b,
    input cin,
    output sum,
    output cout
);
    wire sum1, carry1, carry2;
    halfadder ha1(
        .a(a),
        .b(b),
        .sum(sum1),
        .carry(carry1)
    );
    halfadder ha2(
        .a(sum1),
        .b(cin),
        .sum(sum),
        .carry(carry2)
    );
    or cout_or(cout, carry1, carry2);
endmodule