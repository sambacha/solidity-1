pragma solidity >=0.5.0;

contract Issue083 {
    uint x;

    /// @notice postcondition  !(y > 0) || (x == __verifier_old_uint(x) + 1)
    function f(uint y) public {
        if (y > 0) x++;
    }
}