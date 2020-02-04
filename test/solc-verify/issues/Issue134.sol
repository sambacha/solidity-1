pragma solidity >=0.5.0;

contract A {

  string constant public s1 = "N1";
  string s2 = "N2";

  string title;

  function f(string storage str) internal {
    require(bytes(str).length > 0);
    title = str;
  }

  constructor() public {
    f(s2);
  }
}