// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Base {
    event Created();

    /// @notice emits Created
    constructor() {
        emit Created();
    }
}

contract EmitsEventsModular is Base {
    event Increased();
    event Decreased();

    uint x;

    modifier increases_x {
        _;
        emit Increased();
    }

    /// @notice emits Created
    constructor() Base() {}

    /// @notice emits Increased
    function inc() public increases_x {
        x++;
    }

    /// @notice emits Decreased
    function dec() public {
        x--;
        emit Decreased();
    }

    /// @notice emits Increased
    /// @notice emits Decreased
    function both() public {
        inc();
        dec();
    }
}