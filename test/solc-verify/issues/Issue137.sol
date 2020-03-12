pragma solidity >=0.5.0;

contract Base {
    constructor() public {
        assembly { /* ... */ }
    }
}

contract Derived is Base {
    // Implicit constructor inlines Base constructor,
    // should be a skipped function
}
