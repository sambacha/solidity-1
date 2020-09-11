// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract ArraysMemAliasing {
    function() external payable {
        int[] memory a1 = new int[](2);
        a1[0] = 1;
        a1[1] = 2;
        int[] memory a2 = new int[](1);
        a2[0] = 3;
        // There should be no aliasing, following should hold
        assert(a1.length == 2);
        assert(a1[0] == 1);
        assert(a1[1] == 2);
        assert(a2.length == 1);
        assert(a2[0] == 3);
    }
}