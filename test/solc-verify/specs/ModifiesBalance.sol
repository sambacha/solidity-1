// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract C {

    /// @notice modifies address(this).balance
    function correct1() public payable {
    }

    function incorrect1() public payable { // Modifies this.balance illegally
    }

    /// @notice modifies address(this).balance
    /// @notice modifies forward.balance
    function correct2(address payable forward) public payable {
        require(address(this) != forward);
        forward.transfer(msg.value);
    }

    /// @notice modifies address(this).balance
    function incorrect2(address payable forward) public payable {
        require(address(this) != forward);
        forward.transfer(msg.value); // Illegal
    }

    /// @notice modifies address(this).balance
    /// @notice modifies forward.balance if msg.value < 100
    function correct3(address payable forward) public payable {
        require(address(this) != forward);
        if (msg.value < 100)
            forward.transfer(msg.value);
    }

    /// @notice modifies address(this).balance
    /// @notice modifies forward.balance if msg.value < 100
    function incorrect3(address payable forward) public payable {
        require(address(this) != forward);
        forward.transfer(msg.value); // Illegal, condition missing
    }
}