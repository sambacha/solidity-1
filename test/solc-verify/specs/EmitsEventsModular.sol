pragma solidity >=0.5.0;

contract Base {
    event Created();

    /// @notice emits Created
    constructor() public {
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
    constructor() public Base() {}

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