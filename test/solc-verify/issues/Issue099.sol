// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Test {
  struct S {
    int x;
  }
  S[] m_a;
  function test() external view {
    S[] memory a;
    a = m_a; // Should report meaningful error until not implemented
  }
}
