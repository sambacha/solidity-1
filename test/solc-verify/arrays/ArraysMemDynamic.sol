// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract ArraysMemDynamic {
    function f(uint x) public pure {
        require(x > 0);
        int[] memory a1 = new int[](x);
        assert(a1.length == x);
        assert(a1[0] == 0);
    }
}