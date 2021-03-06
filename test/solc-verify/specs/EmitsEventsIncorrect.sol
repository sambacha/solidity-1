pragma solidity >=0.5.0;

contract EmitsEvents {
    event Increased();
    event Decreased();
    uint x;

    /// @notice emits Decreased
    function increase() public {
        x++;
        emit Increased();
    }

    modifier increases_x {
        emit Increased();
        _;
    }

    function increase_with_modifier() public increases_x {
        x++;
    }
}