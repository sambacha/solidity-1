// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract C {
    int x;
    /// @notice modifies x
    function f() public { x = 0; }
}

contract D is C {
    int x1;

    /// @notice modifies C.x
    function correct1() public { f(); }

    /// @notice modifies x1
    function correct2() public { x1 = 5; }

    function incorrect1() public { f(); }

    function incorrect2() public { x1 = 3; }

    /// @notice modifies x1
    function incorrect3() public { C.x = 3; }
}
