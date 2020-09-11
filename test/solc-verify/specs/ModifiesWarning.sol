// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract C {
    int x;

    /// @notice modifies x
    /// @notice modifies y
    function f(int y) public {
        x = y;
    }
}