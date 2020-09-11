// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract InternalMsgValue {
    /// @notice postcondition r == msg.value
    function f() payable public returns(uint r) {
        return msg.value;
    }

    receive() external payable {
        assert(this.f() == 0);    // External call, msg.value is 0
        assert(f() == msg.value); // Internal call, msg.value is forwarded
    }
}
