pragma solidity>=0.5.0;

contract LocalStorageRepack {
    struct P {
        S s;
    }

    struct S {
        int x;
        T t;
    }

    struct T {
        int z;
    }

    S s1;
    S s2;
    P p;

    function() external payable {
        s1.x = 1;
        s1.t.z = 11;
        s2.x = 2;
        s2.t.z = 22;
        p.s.x = 3;
        p.s.t.z = 33;

        S storage sptr = s1;
        T storage tptr = sptr.t; // Further reference
        assert(tptr.z == 11);

        sptr = s2;
        tptr = sptr.t; // Further reference
        assert(tptr.z == 22);

        sptr = p.s;
        tptr = sptr.t; // Further reference
        assert(tptr.z == 33);

        P storage pptr = p;
        tptr = pptr.s.t; // Further reference
        assert(tptr.z == 33);
    }
}