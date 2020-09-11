// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract C {
    /// @notice postcondition x == -1
    function f() public pure returns(int x) {
        return 2-3;
    }
}