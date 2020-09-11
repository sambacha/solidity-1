// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Issue098 {
    struct S {
        int balance;
    }

    S s;

    /// @notice modifies s.balance
    function f() public {
        s.balance = 5;
    }
}
