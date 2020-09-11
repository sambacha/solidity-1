// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Base {
    int x;

    /// @notice postcondition x > 0
    /// @notice postcondition _x == x
    event ev(int _x);
}

contract Derived is Base {
    int y;

    /// @notice postcondition x > 0 && y > 0
    /// @notice postcondition x == _x && y == _y
    event ev(int _x, int _y);

    /// @notice emits ev
    function f() public {
        x = 1;
        y = 2;
        emit ev(1, 2);
    }

    /// @notice emits ev
    function g() public {
        x = 2;
        emit ev(1);
    }

}