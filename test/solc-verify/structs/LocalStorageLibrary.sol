pragma solidity >=0.5.0;

library L {
    struct S { uint a; }
    function mul(S storage self, uint x) internal returns (uint) {
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
    }
}
