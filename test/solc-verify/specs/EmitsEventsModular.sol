pragma solidity >=0.5.0;

contract EmitsEventsModular {
    /// @notice tracks-changes-in x
    event Increased();
    /// @notice tracks-changes-in x
    event Decreased();

    uint x;

    /// @notice emits Increased
    function inc() public {
        x++;
        emit Increased();
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