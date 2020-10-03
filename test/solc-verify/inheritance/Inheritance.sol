// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Base {
    /**
     * @notice postcondition r == 1
     */
    function f() public pure virtual returns (uint r) {
        return 1;
    }

    /**
     * @notice postcondition r == 2
     */
    function g() public pure returns (uint r) {
        return 2;
    }
}

contract Inheritance is Base {
    /**
     * @notice postcondition r == 3
     */
    function f() public pure override returns (uint r) {
        return 3;
    }

    function h() public pure returns (uint) {
        uint x = f() + g();
        assert(x == 5);
        return x;
    }
}
