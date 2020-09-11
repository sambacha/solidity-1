// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

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

    function testReassign() public {
        x1[0] = 1;
        x2[0] = 2;

        int[] storage s = x1;
        assert(x1[0] == 1);
        assert(x2[0] == 2);
        assert(s[0] == 1);

        s = x2;
        assert(x1[0] == 1);
        assert(x2[0] == 2);
        assert(s[0] == 2);
    }

    function testAssignToStorage() public {
        x1[0] = 1;
        x2[0] = 2;

        int[] storage s = x1;
        assert(x1[0] == 1);
        assert(x2[0] == 2);
        assert(s[0] == 1);

        x2 = s;
        assert(x1[0] == 1);
        assert(x2[0] == 1);
        assert(s[0] == 1);

        x1[0] = 3;
        assert(x1[0] == 3);
        assert(x2[0] == 1);
        assert(s[0] == 3);
    }

    function testAssignToMem() public {
        x1[0] = 1;

        int[] storage s = x1;
        assert(x1[0] == 1);
        assert(s[0] == 1);

        int[] memory m = s;
        assert(x1[0] == 1);
        assert(s[0] == 1);
        assert(m[0] == 1);

        s[0] = 2;
        assert(x1[0] == 2);
        assert(s[0] == 2);
        assert(m[0] == 1);
    }

    receive() external payable {
        testSimple();
        testReassign();
        testAssignToStorage();
        testAssignToMem();
    }
}
