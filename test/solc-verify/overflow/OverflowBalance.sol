// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

/**
 * @notice invariant bal <= address(this).balance
 */
contract Overflow {
    uint256 bal;

    constructor() {
        bal = 0;
    }

    function receive() public payable {
        bal += msg.value; // Should not overflow
    }
}