// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Issue092 {
    struct S {
        int x;
    }
    S s1;

    function f(bool b) public {
        s1.x = 1;
        S memory sm = S(2);
        // Conditional with different data locations for true/false expression
        sm = b ? s1 : sm;
        s1 = b ? sm : s1;

        assert(!b || s1.x == 1);
        assert(!b || sm.x == 1);
        assert(b || sm.x == 2);
        assert(b || s1.x == 1);
    }
}