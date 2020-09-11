// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract A {

  string constant public s1 = "N1";
  string s2 = "N2";

  string title;

  function f(string storage str) internal {
    require(bytes(str).length > 0);
    title = str;
  }

  constructor() {
    f(s2);
  }
}