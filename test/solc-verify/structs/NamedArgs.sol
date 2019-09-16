pragma solidity >=0.5.0;

contract NamedArgs {

  struct S {
    bool f1;
    int f2;
  }

  function() external payable {
    S memory s1 = S({f1: true, f2: 10});
    S memory s2 = S({f2: 10, f1: true});
    assert(s1.f1 == s2.f1);
    assert(s1.f2 == s2.f2);
  }

}