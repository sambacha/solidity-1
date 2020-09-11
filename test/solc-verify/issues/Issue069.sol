// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Base {
    constructor() {
        f();
    }

    function f() public pure {}
}

contract Derived is Base {

}
