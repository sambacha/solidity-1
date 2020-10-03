// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract MappingInArray {
    mapping(address=>int)[2] arr;

    constructor() {
        assert(arr[0][msg.sender] == 0);
    }

    receive() external payable {
        arr[0][msg.sender] = 5;
        int x = arr[0][msg.sender];
        assert(x == 5);
    }
}
