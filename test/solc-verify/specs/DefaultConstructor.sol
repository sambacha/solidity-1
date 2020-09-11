// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

/**
 * @notice invariant x == y
 */
contract DefaultConstructor {
    // The generated default constructor (setting the initial values) should fail
    uint x = 0;
    uint y = 1;

    function add(uint amount) public {
        x += amount;
        y += amount;
    }
}