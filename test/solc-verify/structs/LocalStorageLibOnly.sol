pragma solidity >=0.5.0;

library L {

    struct S { int x; }

    /// @notice postcondition self.x >= 0
    function square(S storage self) public {
        self.x *= self.x;
    }

    /**
     * @notice postcondition a.x == __verifier_old_int(b.x) || a.x == 0
     * @notice postcondition b.x == __verifier_old_int(a.x) || b.x == 0
     */
    function swap_if_different(S storage a, S storage b) public {
        a.x = a.x + b.x;
        b.x = a.x - b.x;
        a.x = a.x - b.x;
    }
}
