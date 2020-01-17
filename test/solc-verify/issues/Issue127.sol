pragma solidity >=0.5.0;
contract A {
  address owner;
  constructor(address _owner) public {
    owner = _owner;
  }
}
contract B {
  A a;
  constructor() public {
    a = new A(msg.sender);
  }
}