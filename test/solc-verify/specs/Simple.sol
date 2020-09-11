// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

/**
 * @notice invariant x == y
 * @notice some random comment
 */
contract Simple {
    uint x;
    uint y;

    constructor() {
        x = y = 0;
    }

    function add(uint amount) public {
        x += amount;
        y += amount;
    }

    function add_incorrect(uint amount) public {
        x += amount;
        y -= amount; // ERROR
    }

    function add_incorrect_2(uint amount) public {
        x += amount;
        add(amount); // ERROR, invariant does not hold when calling function
        y += amount;
    }
}