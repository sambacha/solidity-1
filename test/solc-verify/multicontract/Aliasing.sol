// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Aliasing_A {
    int x;

    /** @notice postcondition x == 0 */
    function reset() public { x = 0; }

    /** @notice postcondition r == x */
    function get() public view returns (int r) { return x; }
}

contract Aliasing is Aliasing_A {

    Aliasing_A a1;
    Aliasing_A a2;

    constructor() {
        a1 = new Aliasing_A();
        a2 = new Aliasing_A();
    }

    receive() external payable {
        require(a1 != a2);
        a1.reset();
        assert(a1.get() == 0); // Should hold
        a2.reset();
        assert(a2.get() == 0); // Should hold
        reset();
        assert(get() == 0); // Should hold
        assert(a1.get() == 0); // Should hold
        assert(a2.get() == 0); // Should hold
    }
}
