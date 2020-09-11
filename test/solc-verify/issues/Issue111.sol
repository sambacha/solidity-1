// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract A {
    /// @notice postcondition r == 0
    function f() public returns (int r);

    /// @notice postcondition r == 0
    function g() public payable returns (int r);

    function test() public payable {
        int fr = f();
        assert(fr == 0);
        int gr = g();
        assert(gr == 0);
    }
}