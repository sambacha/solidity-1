// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Issue097 {
    int[] arr;

    constructor() {
        arr.push(0);
        assert(arr[arr.length-1] == 0); // Should hold
    }

    receive() external payable {
        arr.push(0);
        assert(arr[arr.length-1] == 0); // Should hold
    }
}
