// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract C {
    int x;
    /// @notice postcondition C.x == 0
    function f() public { x = 0; }
}
