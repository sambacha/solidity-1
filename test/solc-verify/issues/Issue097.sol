pragma solidity >=0.5.0;

contract Issue097 {
    int[] arr;

    constructor() public {
        arr.length += 1;
        assert(arr[arr.length-1] == 0); // Should hold
    }

    function() external payable {
        arr.length += 1;
        assert(arr[arr.length-1] == 0); // Should hold
    }
}