// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

/** @notice invariant x >= 0 */
contract NE_A {
    int private x;

    /** @notice postcondition x == 0 */
    constructor() {}

    function incr() public { x++; }

    /** @notice postcondition r == x */
    function getX() public view returns (int r) { return x; }

    /** @notice postcondition x == _x */
    function setX(int _x) public { require(_x >= 0); x = _x; }
}

contract NewExpr {
    function single() public {
        NE_A a = new NE_A();
        a.incr();
        assert(a.getX() >= 0); // Should hold
    }

    function multiple() public {
        NE_A a1 = new NE_A();
        assert(a1.getX() == 0); // Should hold
        a1.setX(1);
        assert(a1.getX() == 1); // Should hold
        NE_A a2 = new NE_A();
        assert(a2.getX() == 0); // Should hold
        assert(a1.getX() == 1); // Should hold
    }

    receive() external payable {
        single();
        multiple();
    }
}
