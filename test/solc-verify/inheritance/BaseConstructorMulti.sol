// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract A {
    int public n;
    constructor(int x) {
        n += x;
    }
}

contract B is A(1) {

}

contract BaseConstructorMulti is A, B {
    constructor() {
        assert(n == 1); // Constructor of A is only called once
    }

    receive() external payable { } // Needed for detecting as a truffle test case
}
