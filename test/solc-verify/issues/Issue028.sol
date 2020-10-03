// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Issue028 {
    function f() pure public returns (uint, uint) {
        ((2**270) / 2**100, 1);
    }
}
