// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

/**
 * @notice invariant bal <= address(this).balance
 */
contract OverflowBalance {
    uint256 bal;

    constructor() {
        bal = 0;
    }

    receive() external payable {
        bal += msg.value; // Should not overflow
    }
}
