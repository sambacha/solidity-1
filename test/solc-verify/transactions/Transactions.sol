// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Base {
    mapping(address=>int) public m;

    /// @notice postcondition m[msg.sender] == v
    function f(int v) public {
        m[msg.sender] = v;
    }
}

contract Transactions is Base {

    receive() external payable {
        require(msg.sender != address(this));

        f(4); // Internal
        assert(m[msg.sender] == 4);

        Base.f(5); // Internal
        assert(m[msg.sender] == 5);

        this.f(6); // External
        assert(m[address(this)] == 6);
    }
}
