pragma solidity >=0.5.0;

contract StructsMemAliasing {
    struct S {
        int x;
    }

    function() external payable {
        S memory sm1 = S(1);
        S memory sm2 = S(2);
        S memory sm3 = sm1;

        assert(sm1.x == 1);
        assert(sm2.x == 2);
        assert(sm3.x == 1);

        sm1.x = 3;

        assert(sm1.x == 3);
        assert(sm2.x == 2);
        assert(sm3.x == 3);
    }
}