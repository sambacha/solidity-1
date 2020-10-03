// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract A {
    uint public myVar;

    function someFunc(uint param) view public returns (uint) {
        return myVar + param;
    }
}

contract B {
    A other;

    function callOtherFunc() view public returns (uint) {
        return other.someFunc(10);
    }

    function accessOtherMember() view public returns (uint) {
        return other.myVar();
    }
}
