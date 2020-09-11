// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract A {
  uint constant C = 1;
  function f(address a) public returns (bool) {
    (bool ok, ) = a.call("");
    return ok;
  }
}
