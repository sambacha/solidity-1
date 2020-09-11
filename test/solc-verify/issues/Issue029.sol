// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract C {
    function f() pure public {
        uint super = 3;
        uint this = 4;
    }

    function super() pure public{

    }

    function g() pure public {
        super();
    }
}

contract C2 {
    struct S {
        int x;
    }

    S super;

    function f(int y) public {
        super.x = y;
    }
}
