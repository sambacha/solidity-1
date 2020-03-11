pragma solidity >=0.5.0;

contract EmitsEvents {

    /// @notice tracks-changes-in x
    event Increased();
    uint x;

    /// @notice emits Increased
    function increase() public {
        x++;
        emit Increased();
    }
}