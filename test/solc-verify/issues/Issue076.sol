pragma solidity >=0.5.0;

contract Issue076 {
    function f(int x, bool y) private pure {
        assert(x == 1);
        assert(y);
    }
    function() external payable {
        f(1, true);
        f({y:true, x:1});
    }
}