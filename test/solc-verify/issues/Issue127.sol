// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract A {
  address owner;
  constructor(address _owner) {
    owner = _owner;
  }
}
contract B {
  A a;
  constructor() public {
    a = new A(msg.sender);
  }
}
