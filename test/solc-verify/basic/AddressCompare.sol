pragma solidity >=0.5.0;
contract A {
  address x;
  function add(address y) public {
    if (y > x) {
      x = y;
    }
  }
}