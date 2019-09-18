pragma solidity >=0.5.0;

library L {
    struct S { int a; }

    /// @notice postcondition self.a >= 0
    function square(S storage self) public {
        self.a *= self.a;
    }
}