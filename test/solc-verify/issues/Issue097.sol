// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Issue097 {
    int[] arr;

    constructor() public {
        arr.length += 1;
        assert(arr[arr.length-1] == 0); // Should hold
    }

    receive() external payable {
        arr.length += 1;
        assert(arr[arr.length-1] == 0); // Should hold
    }
}
