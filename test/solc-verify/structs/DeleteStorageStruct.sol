// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract DeleteStorageStruct {
    struct T {
        int z;
    }
    struct S {
        int x;
        T t;
    }

    S s;

    constructor() public {
        assert(s.t.z == 0);
        assert(s.x == 0);
        s.t.z = 1;
        s.x = 2;
        assert(s.t.z == 1);
        assert(s.x == 2);
        delete s;
        assert(s.t.z == 0);
        assert(s.x == 0);
    }

    function() external payable {
    }
}