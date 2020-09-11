// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract A {
  mapping (string=>int) m;
  function f(string memory id) public view returns (int x) {
    x = m[id];
  }
}