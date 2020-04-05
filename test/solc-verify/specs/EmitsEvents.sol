pragma solidity >=0.5.0;

contract EmitsEvents {

    event Increased();
    uint x;

    /// @notice emits Increased
    function increase() public {
        x++;
        emit Increased();
    }
}