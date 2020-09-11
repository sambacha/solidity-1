// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract C1 {
    constructor() {
        // Should hold
        assert(address(this).balance >= 0);
    }
}

contract C2 {
    constructor() {
        // Might not hold
        assert(address(this).balance == 0);
    }
}
