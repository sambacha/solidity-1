# solc-verify

This is an extended version of the compiler (v0.5.16) that is able to perform **automated formal verification** on Solidity smart contracts using **specification annotations** and **modular program verification**. More information can be found in this readme and in our [papers](#papers).

First, we present how to [build, install](#build-and-install) and [run](#running-solc-verify) solc-verify including its options.
Then we illustrate the features of solc-verify through some [examples](#examples).
Finally we discuss available [specification annotations](#specification-annotations) and interpreting
[verification results](#verification-and-results) in more detail.

## Build and Install

Solc-verify is mainly developed and tested on Ubuntu and OS X. It requires [CVC4](http://cvc4.cs.stanford.edu) (or [Z3](https://github.com/Z3Prover/z3)) and [Boogie](https://github.com/boogie-org/boogie) as a verification backend.
On a standard Ubuntu system (18.04), solc-verify can be built and installed as follows.
Alternatively, you can also use our [docker image](docker/README.md) to quickly try the tool.

**[CVC4](http://cvc4.cs.stanford.edu)** (>=1.6 required)
```
wget http://cvc4.cs.stanford.edu/downloads/builds/x86_64-linux-opt/cvc4-1.7-x86_64-linux-opt
chmod a+x cvc4-1.7-x86_64-linux-opt
sudo cp cvc4-1.7-x86_64-linux-opt /usr/local/bin/cvc4
```

CVC4 (or Z3) should be on the `PATH`.

**[.NET Core runtime 3.1](https://docs.microsoft.com/dotnet/core/install/linux-package-managers)** (required for Boogie)
```
wget -q https://packages.microsoft.com/config/ubuntu/18.04/packages-microsoft-prod.deb -O packages-microsoft-prod.deb
sudo dpkg -i packages-microsoft-prod.deb
sudo add-apt-repository universe
sudo apt-get update
sudo apt-get install apt-transport-https
sudo apt-get update
sudo apt-get install dotnet-runtime-3.1
```

**[Boogie](https://github.com/boogie-org/boogie)**
```
wget https://github.com/boogie-org/boogie/releases/download/v2.5.10/Boogie.2.5.10.nupkg
unzip Boogie.2.5.10.nupkg -d Boogie
```

**solc-verify**
```
sudo apt install python3-pip -y
pip3 install psutil
git clone https://github.com/SRI-CSL/solidity.git
cd solidity
./scripts/install_deps.sh
mkdir build
cd build
cmake -DBOOGIE_BIN="../../Boogie/tools/netcoreapp3.1/any/" ..
make
sudo make install
cd ../..
```

(Change `-DBOOGIE_BIN` if Boogie is located in a different directory.)

## Running solc-verify

After successful installation, solc-verify can be run by `solc-verify.py <solidity-file>`. You can type `solc-verify.py -h` to print the optional arguments, but we also list them below.

- `-h`, `--help`: Show help message and exit.
- `--timeout TIMEOUT`: Timeout for running the Boogie verifier in seconds (default is 10).
- `--arithmetic {int,bv,mod,mod-overflow}`: Encoding of the arithmetic operations (see [paper](https://arxiv.org/abs/1907.04262) for more details):
  - `int` is SMT (unbounded, mathematical) integer mode, which is scalable and well supported by solvers, but do not capture exact semantics (e.g., overflows, unsigned numbers)
  - `bv` is SMT bitvector mode, which is precise but might not scale for large bit-widths
  - `mod` is modular arithmetic mode, encoding arithmetic operations using mathematical integers with range assertions and precise wraparound semantics
  - `mod-overflow` is modular arithmetic with overflow checking enabled
- `--modifies-analysis`: State variables and balances are checked for modifications if there are modification annotations or if this flag is explicitly given.
- `--output OUTPUT`: Output directory where the intermediate (e.g., Boogie) files are created (tmp directory by default).
- `--verbose`: Print all output of the compiler and the verifier.
- `--smt-log SMTLOG`: Log the inputs given by Boogie to the SMT solver into a file (not given by default).
- `--errors-only`: Only display error messages and omit displaying names of correct functions (not given by default).
- `--show-warnings`: Display warning messages (not given by default).
- `--solc SOLC`: Path to the Solidity compiler to use (which must include our Boogie translator extension) (by default it is the one that includes the Python script).
- `--boogie BOOGIE`: Path to the Boogie verifier binary to use (by default it is the one given during building the tool).
- `--solver {z3,cvc4}`: SMT solver used by the verifier (default is detected during compile time).
- `--solver-bin`: Path to the solver to be used, if not given, the solver is searched on the system path (not given by default).

## Examples

Some examples are located under the `test/solc-verify/examples`.

### Specifictaion Annotations

This example ([`Annotations.sol`](test/solc-verify/examples/Annotations.sol)) presents some of the available specification annotations. A _contract-level invariant_ (line 3) ensures that `x` and `y` are always equal. Contract-level annotations are added as both _pre-_ and _postconditions_ to public functions. Non-public functions (such as `add_to_x`) are not checked against contract-level invariants, but can be annotated with pre- and post-conditions explicitly. By default, non-public functions are _inlined_ to a depth of 1. Loops can be annotated with _loop invariants_. Furthermore, functions can be annotated with the state variables that they can _modify_ (including conditions). This contract is correct and can be verified by the following command:
```
solc-verify.py test/solc-verify/examples/Annotations.sol
```
Note that it is also free of _overflows_, since the programmer included an explicit check in line 13. Solc-verify can detect this and avoid a false alarm:
```
solc-verify.py test/solc-verify/examples/Annotations.sol --arithmetic mod-overflow
```
However, removing that check and running solc-verify with overflow checks will report the potential overflow.

### SimpleBank

This is the simplified version of the [infamous DAO hack](https://link.springer.com/chapter/10.1007/978-1-4842-3081-7_6), illustrating the reentrancy issue. There are two versions of the `withdraw` function (line 13). In the incorrect version ([`SimpleBankReentrancy.sol`](test/solc-verify/examples/SimpleBankReentrancy.sol)) we first transfer the money and then reduce the balance of the sender, allowing a reentrancy attack. The operations in the correct version ([`SimpleBankCorrect.sol`](test/solc-verify/examples/SimpleBankCorrect.sol)) are the other way around, preventing the reentrancy attack. The contract is annotated with a contract level invariant (line 4) ensuring that the balance of the contract is at least the sum of individual balances. Using this invariant we can detect the error in the incorrect version (invariant does not hold when the reentrant call is made) and avoid a false alarm in the correct version (invariant holds when the reentrant call is made).
```
solc-verify.py test/solc-verify/examples/SimpleBankReentrancy.sol
solc-verify.py test/solc-verify/examples/SimpleBankCorrect.sol
```

[`SumOverStructMember.sol`](test/solc-verify/examples/SumOverStructMember.sol) presents a modified version of the simple bank, where the accounts are complex structures and the sum is expressed over a member of this structure.
```
solc-verify.py test/solc-verify/examples/SumOverStructMember.sol
```

### BecToken

This example ([`BecTokenSimplifiedOverflow.sol`](test/solc-verify/examples/BecTokenSimplifiedOverflow.sol)) presents a part of the BecToken, which had an [overflow issue](https://nvd.nist.gov/vuln/detail/CVE-2018-10299). It uses the `SafeMath` library (from [OpenZeppelin](https://github.com/OpenZeppelin/openzeppelin-contracts/)) for most operations to prevent overflows, except for a multiplication in `batchTransfer` (line 61). The function transfers a given `_value` to a given number of `_receivers`. It first reduces the balance of the sender with the product of the value and the number of receivers and then transfers the value to each receiver in a loop. If the product overflows, a small product will be deducted from the sender, but large values will be transferred to the receivers. Solc-verify can detect this issue by the following command (using CVC4):
```
solc-verify.py test/solc-verify/examples/BecTokenSimplifiedOverflow.sol --arithmetic mod-overflow --solver cvc4
```
In the correct version ([`BecTokenSimplifiedCorrect.sol`](test/solc-verify/examples/BecTokenSimplifiedCorrect.sol)), the multiplication in line 61 is replaced by the `mul` operation from `SafeMath`, making the contract safe. Solc-verify can not only prove the absence of overflows, but also the contract invariant (sum of balances equals to total supply, line 34) and the loop invariant (line 67) including nonlinear arithmetic over 256-bit integers:
```
solc-verify.py test/solc-verify/examples/BecTokenSimplifiedCorrect.sol --arithmetic mod-overflow --solver cvc4
```

### Storage

This example ([`Storage.sol`](test/solc-verify/examples/Storage.sol)) presents a simple storage example, where each user can set, update or clear their data (represented as an integer) in the storage. The owner can clear any data. This example demonstrates annotation possibilities (such as fine grained modifications) over complex datatypes (such as structs and mappings).
```
solc-verify.py test/solc-verify/examples/Storage.sol
```

## Specification Annotations

Specification annotations must be included in special documentation comments (`///` or `/** */`) and must start with the special doctag `@notice`.
They must be side-effect free Solidity expressions and can refer to variables within the scope of the annotated element. Functions cannot be called in the annotations, except for getters. The currently available annotations are listed below. We try to keep the language simple to enable automation, but it is evolving based on user input.

See the contracts under `test/solc-verify/examples` for examples.

- **Function pre/postconditions** (`precondition <EXPRESSION>` / `postcondition <EXPRESSION>`) can be attached to functions. Preconditions are assumed before executing the function and postconditions are checked (asserted) in the end. The expression can refer to variables in the scope of the function. The postcondition can also refer to the return value if it is named.
- **Contract level invariants**  (`invariant <EXPRESSION>`) can be attached to contracts. They are included as both a pre- and a postcondition for each _public_ function. The expression can refer to state variables in the contract (and its balance).
- **Loop invariants**  (`invariant <EXPRESSION>`) can be attached to _for_ and _while_ loops. The expression can refer to variables in scope of the loop, including the loop counter.
- **Modification specifiers** (`modifies <TARGET> [if <CONDITION>]`) can be attached to functions. The target can be a (1) state variable, including index and member accesses or (2) a balance of an address in scope. Note however, that balance changes due to gas cost or miner rewards are currently not modeled. Optionally, a condition can also be given. Variables in the condition refer to the old values (i.e., before executing the function). Modification specifications will be checked at the end of the function (whether only the specified variables were modified). See [`Storage.sol`](test/solc-verify/examples/Storage.sol) for examples.
- Contract and loop invariants can refer to a special **sum function over collections** (`__verifier_sum_int` or `__verifier_sum_uint`). The argument must be an array/mapping state variable with integer values, or must point to an integer member if the array/mapping contains structures (see [`SumOverStructMember.sol`](test/solc-verify/examples/SumOverStructMember.sol)).
- Postconditions can refer to the **old value** of a variable (before the transaction) using `__verifier_old_<TYPE>` (e.g., `__verifier_old_uint(...)`).
- Specifications can refer to a special **equality predicate** (`__verifier_eq`) for reference types such as structures, arrays and mappings (not comparable with the standard Solidity operator `==`). It takes two arguments with the same type. For storage data location it performs a deep equality check, for other data locations it performs a reference equality check.

## Verification and Results
Solc-verify targets _functional correctness_ of contracts with respect to _completed transactions_ and different types of _failures_.
An _expected failure_ is a failure due to an exception deliberately thrown by the developer (e.g., `require`, `revert`). An _unexpected failure_ is any other failure (e.g., `assert`, overflow).
Solc-verify performs modular verification by checking for each public function whether it can fail due to an unexpected failure or violate its specification in any completed transaction.

The output for each function is `OK`, `ERROR` or `SKIPPED`.
If a function contains any errors, solc-verify lists them below.
If a function contains any unsupported features it is skipped and treated as if it could modify any state variable arbitrarily (safe over-approximation).
However, skipped functions can be specified with annotations, which will be assumed to hold during verification.
Finally, solc-verify lists warnings if some abstraction was applied that might introduce false alarms.

For example, running solc-verify on [VerifResults.sol](test/solc-verify/examples/VerifResults.sol)
```
solc-verify.py test/solc-verify/examples/VerifResults.sol
```
results in
```
VerifResults::[constructor]: OK
VerifResults::set_correct: OK
VerifResults::set_incorrect: ERROR
 - test/solc-verify/examples/VerifResults.sol:22:5: Postcondition 'x == x1' might not hold at end of function.
VerifResults::use_unsupported: OK
VerifResults::unsupported: SKIPPED
Use --show-warnings to see 3 warnings.
Some functions were skipped. Use --verbose to see details.
Errors were found by the verifier.
```

The `constructor` and `set_correct` are correct.
However, `set_incorrect` has a postcondition that can fail.
Furthermore, `unsupported` contains some unsupported features and is skipped.
Nevertheless, it is annotated so the function `use_unsupported` that calls it can still be proved correct.

## Papers
- [ESOP 2020](https://www.etaps.org/2020/esop): [SMT-Friendly Formalization of the Solidity Memory Model](https://arxiv.org/abs/2001.03256)
  _Formalization of reference types (e.g., arrays, mappings, structs) and the memory model (storage and memory data locations)._
- [VSTTE 2019](https://sri-csl.github.io/VSTTE19/): [solc-verify: A Modular Verifier for Solidity Smart Contracts](https://arxiv.org/abs/1907.04262)
  _An overview of the modular verification approach including the specification annotations and the translation to Boogie._
