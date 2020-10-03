// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract A {
    uint constant x = 2;

    function f() public pure {
        assert(x == 2);
    }
}

contract B is A {
    function g() public pure {
        assert(A.x == 2);
    }
}
