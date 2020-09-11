// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract A {
  address x;
  function add(address y) public {
    if (y > x) {
      x = y;
    }
  }
}
