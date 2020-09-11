// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Issue004 {
    bytes2 x;
    function f(bytes2 b) public view {
        b[uint8(x[1])];
    }
}
