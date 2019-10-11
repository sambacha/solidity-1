pragma solidity ^0.5.0;

contract C {
    function test() public pure {
        int[] memory a;
        assert(a.length == 0);
    }
}