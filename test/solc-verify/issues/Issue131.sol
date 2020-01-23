pragma solidity >=0.5.0;

contract A {

  mapping(address=>uint) b1;
  mapping(address=>uint) b2;
  mapping(address=>uint) b3;

  modifier min(uint m) {
    address _sender = msg.sender;
    if (b1[_sender] >= m) {
      _;
    } else if (b2[_sender] >= m) {
      _;
    }
  }

  function f() min(100) public {
    address _sender = msg.sender;
    b3[_sender] += 1;
  }
}
