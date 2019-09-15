pragma solidity >=0.5.0;

contract A {
    int x;
    int y;

    /// @notice precondition x > 0
    /// @notice precondition y == 0
    function f() public {
        g(); // x should be havoced
        assert(x == 0); // Should hold
        assert(y == 0); // Should hold
        assert(false); // Should fail, just to test that it is reached.
                       // If x is not havoced, the precondition x>0 is
                       // contradicts the postcondition x==0 and the
                       // verifier "silently" stops
    }

    /// @notice modifies x
    /// @notice postcondition x == 0
    function g() public;
}
