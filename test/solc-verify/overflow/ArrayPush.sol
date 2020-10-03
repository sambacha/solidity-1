// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract ArrayPush {
    int[] a;
    function add(int x) public returns (uint index) {
        a.push(x); // Should not report overflow for length
        assert(a.length > 0); // Should hold after a push
        index = a.length - 1; // Should not report underflow
    }
}
