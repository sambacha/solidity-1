pragma solidity>=0.5.0;

contract ArrayLocalStorage {
    int[] x1;
    int[] x2;
    bool[] b;

    constructor() public {
        x1.push(0);
        x2.push(0);
    }
    
    function testSimple() public {
        x1[0] = 1;
        x2[0] = 2;

        int[] storage s = x1;
        assert(x1[0] == 1);
        assert(x2[0] == 2);
        assert(s[0] == 1);

        x1[0] = 3;
        assert(x1[0] == 3);
        assert(x2[0] == 2);
        assert(s[0] == 3);

        s[0] = 4;
        assert(x1[0] == 4);
        assert(x2[0] == 2);
        assert(s[0] == 4);
    }

    function() external payable {
        testSimple();
    }
}