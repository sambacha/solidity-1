// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract C {
    function test() public pure {
        int[] memory a;
        assert(a.length == 0);
    }
}
