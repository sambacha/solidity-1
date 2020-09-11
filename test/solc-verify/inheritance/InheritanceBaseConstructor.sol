// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

/// @notice invariant x == 42
contract C {
    int x;
    constructor() public { x = 42; }
}

/// @notice invariant C.x == 42
contract D is C { }

/// @notice invariant C.x == 42
contract E is D {
    constructor() public { }
}