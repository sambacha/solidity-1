pragma solidity >=0.5.0;
contract A {
  mapping (string=>int) m;
  function f(string memory id) public view returns (int x) {
    x = m[id];
  }
}