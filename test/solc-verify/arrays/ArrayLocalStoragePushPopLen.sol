// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract ArrayLocalStoragePushPopLen {
    int[] arr;

    function() external payable {
        require(arr.length == 0);

        int[] storage s = arr;
        s.push(5);
        assert(s.length == 1);
        assert(s[0] == 5);
        s.pop();
        assert(s.length == 0);
    }
}