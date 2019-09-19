pragma solidity>=0.5.0;

contract C {
    uint40 x;
    /// @notice postcondition x == __verifier_old_uint(x) + 1
    function f() public {
        x++;
    }
}