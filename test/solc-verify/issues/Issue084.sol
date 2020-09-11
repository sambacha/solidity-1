// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Issue084 {
    struct S {
        uint x;
    }

    S s;

    function() external payable {
        assert(s.x >= 0);
    }
}