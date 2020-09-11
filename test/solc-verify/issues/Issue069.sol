// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Base {
    constructor() public {
        f();
    }

    function f() public pure {}
}

contract Derived is Base {

}
