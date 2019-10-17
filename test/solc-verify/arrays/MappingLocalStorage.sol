pragma solidity >=0.5.0;

contract MappingLocalStorage {
    struct S {
        int x;
    }
    mapping(int=>S) m1;
    mapping(int=>S) m2;

    function() external payable {
        m1[0].x = 1;
        m2[5].x = 3;
        mapping(int=>S) storage m = m1;
        assert(m[0].x == 1); // m points to m1
        m[0].x = 2; // overwriting m modifies m1
        assert(m1[0].x == 2);
        m = m2;
        assert(m[5].x == 3); // m now points to m2
    }
}