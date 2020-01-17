pragma solidity >=0.5.0;
contract A {
  uint constant C = 1;
  function f(address a) public returns (bool) {
    (bool ok, ) = a.call("");
    return ok;
  }
}