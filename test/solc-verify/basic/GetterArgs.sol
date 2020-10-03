// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract A {
    mapping(address=>int) public m1;
    mapping(address=>mapping(int=>int)) public m2;
    mapping(address=>int)[] public ma;
    int[] public a;

    /// @notice postcondition m1[key] == value
    function setm1(address key, int value) public {
        m1[key] = value;
    }

    /// @notice postcondition m2[key1][key2] == value
    function setm2(address key1, int key2, int value) public {
        m2[key1][key2] = value;
    }

    /// @notice postcondition a[key] == value
    function seta(uint key, int value) public {
        a[key] = value;
    }

    /// @notice postcondition ma[key1][key2] == value
    function setma(uint key1, address key2, int value) public {
        ma[key1][key2] = value;
    }
}

contract B {
    A a;

    function f() public {
        a.setm1(msg.sender, 1);
        assert(a.m1(msg.sender) == 1);
        a.setm2(msg.sender, 2, 3);
        assert(a.m2(msg.sender, 2) == 3);
        a.seta(5, 6);
        assert(a.a(5) == 6);
        a.setma(7, msg.sender, 8);
        assert(a.ma(7, msg.sender) == 8);
    }
}
