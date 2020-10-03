// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Issue065 {
    /// @notice postcondition r == 1
    function value() public pure returns (uint r) {
        return 1;
    }

    receive() external payable {
        assert(Issue065(this).value() == 1);
    }
}
