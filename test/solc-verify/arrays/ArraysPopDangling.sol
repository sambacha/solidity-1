pragma solidity>=0.5.0;

contract ArraysPopDangling {
    struct S { int x; }

    S[] arr;

    function() external payable {
        arr.push(S(1));
        S storage s = arr[arr.length-1];
        arr.pop();
        assert(s.x == 0);
    }
}