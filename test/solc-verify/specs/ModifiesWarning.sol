pragma solidity >=0.5.0;

contract C {
    int x;

    /// @notice modifies x
    /// @notice modifies y
    function f(int y) public {
        x = y;
    }
}