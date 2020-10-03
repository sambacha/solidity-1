// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Issue102 {
    struct S {
        int x;
        bool b;
    }

    receive() external payable {
        S memory s; // Allocate new and set default values
        assert(s.x == 0);
        assert(s.b == false);
    }
}
