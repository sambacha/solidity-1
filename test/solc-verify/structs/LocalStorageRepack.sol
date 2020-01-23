pragma solidity>=0.5.0;

contract LocalStorageRepack {
    struct S {
        int x;
        T t;
    }

    struct T {
        int z;
    }

    S s1;
    S s2;

    function() external payable {
        s1.x = 1;
        s1.t.z = 11;
        s2.x = 2;
        s2.t.z = 22;

        S storage sptr = s1;
        T storage tptr = sptr.t;

        assert(tptr.z == 11);

        sptr = s2;
        tptr = sptr.t;

        assert(tptr.z == 22);
    }
}