pragma solidity >=0.5.0;

library L {
    struct S { uint a; }
    // public --> specified
    /// @notice postcondition self.a == __verifier_old_uint(self.a) * x
    function mul(S storage self, uint x) public returns (uint) {
        return self.a *= x;
    }

    // internal --> inlined
    function mul2(S storage self, uint x) internal returns (uint) {
        return self.a *= x;
    }
}

contract LocalStorageLibrary {
    using L for L.S;
    L.S x;
    function f() public {
        x.a = 2;
        x.mul(3); // Non-static call
        assert(x.a == 6);

        x.a = 3;
        L.mul(x, 4); // Static call
        assert(x.a == 12);

        x.a = 4;
        x.mul2(5); // Non-static call
        assert(x.a == 20);

        x.a = 6;
        L.mul2(x, 7); // Static call
        assert(x.a == 42);
    }
}
