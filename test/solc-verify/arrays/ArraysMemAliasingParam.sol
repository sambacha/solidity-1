pragma solidity >=0.5.0;

contract ArraysMemAliasingParam {
    function f(int[] memory a1, int[] memory a2) public pure {
        require(a1.length > 0);
        require(a2.length > 0);
        a1[0] = 1;
        a2[0] = 1;
        int[] memory am = new int[](1);
        am[0] = 2;
        assert(a1[0] == 1);
        assert(a2[0] == 1);
        assert(am[0] == 2);
        a1[0] = 3;
        assert(a1[0] == 3);
        assert(am[0] == 2);
        assert(a2[0] == 1); // Can fail because a1 and a2 can alias
    }
}