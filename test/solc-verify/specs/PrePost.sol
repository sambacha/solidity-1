// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract PrePost {
    /**
     * @notice precondition y >= 0
     * @notice postcondition result >= x
     */
    function test(int x, int y) public pure returns(int result) {
        return x + y;
    }

    function violatePrecondition() public pure {
        test(2, -3);
    }
}