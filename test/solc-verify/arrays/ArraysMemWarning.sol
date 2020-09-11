// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract ArraysMemDynamic {
    struct S {int x; }
    function f(uint len) public pure {
        require(len > 0);
        S[] memory a1 = new S[](len);
        assert(a1.length == len);
        assert(a1[0].x == 0);
    }
}
