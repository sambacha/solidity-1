pragma solidity >=0.5.0;

contract Base {
    uint b;

    /// @notice tracks-changes-in b
    event Created();

    /// @notice emits Created
    constructor(uint _b) public {
        b = _b;
        emit Created();
    }
}

contract EmitsEventsModular is Base {
    /// @notice tracks-changes-in x
    event Increased();
    /// @notice tracks-changes-in x
    event Decreased();

    uint x;

    modifier increases_x {
        _;
        emit Increased();
    }

    /// @notice emits Created
    constructor() public Base(0) {}

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