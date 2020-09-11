// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Base {
    constructor() public {
        assembly { /* ... */ }
    }
}

contract Derived is Base {
    // Implicit constructor inlines Base constructor,
    // should be a skipped function
}
