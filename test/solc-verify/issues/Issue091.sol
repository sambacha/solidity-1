// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Issue091 {
    function() external payable {
        uint8 x = 0;
        uint16 y = 1;
        uint32 z = x == 0 ? x : y;
        assert(z == 0);
    }
}