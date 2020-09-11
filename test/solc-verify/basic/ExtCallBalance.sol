// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract ExtCallBalance {
    function g() public payable {
    }

    function f() public {
        uint oldBal = address(this).balance; // Old balance
        msg.sender.call(""); // We make an external call with no value

        // But this should fail because callbacks can modify balance
        assert(oldBal == address(this).balance);
    }
}