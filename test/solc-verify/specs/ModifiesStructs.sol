// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract ModifiesStructs {

    struct T {
        int z;
    }

    struct S {
        int x;
        T t;
    }

    S s;
    mapping(address=>S) ss;

    /// @notice modifies s.x
    /// @notice modifies s.t.z
    function f1() public {
        s.x = 1;
        s.t.z = 5;
    }

    /// @notice modifies s.x
    /// @notice modifies s if v > 0
    function f2(uint v) public {
        s.x = 1;
        if (v > 0) s.t.z = 5;
    }

    /// @notice modifies ss[msg.sender].t.z
    function f3() public {
        ss[msg.sender].t.z = 5;
    }

    /// @notice modifies ss[msg.sender].t
    function f4() public {
        ss[msg.sender].t.z = 5;
    }

    /// @notice modifies ss[msg.sender].t.z
    function f5() public {
        ss[msg.sender].x = 7; // ILLEGAL
    }
}
