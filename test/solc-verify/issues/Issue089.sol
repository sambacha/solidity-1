// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Issue089 {
    function sign(int x) public pure returns (string memory result) {
        result = x > 0 ? "pos" : "neg";
    }
    function sign2(int x) public pure returns (string memory result) {
        string memory pos = "pos";
        result = x > 0 ? pos : "neg";
    }

    struct S {
        int x;
    }
    S s1;

    function f(int x) public {
        s1.x = 1;
        S memory sm = S(2);
        sm = x > 0 ? s1 : sm;
        s1 = x > 0 ? s1 : sm;
    }
}
