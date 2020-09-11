// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract C {
    /// @notice postcondition x == 0
    function f() public pure returns (uint8 x) {
        uint8 y = 255;
        return y + 1;
    }
}