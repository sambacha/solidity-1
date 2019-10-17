pragma solidity >=0.5.0;

contract Test {

    int[][] s;

    function test() public {
        require(s.length > 0 && s[0].length > 0);
        s[0][0] = 1;
        int[][] storage sl = s;
        assert(sl[0][0] == 1);
    }
}
