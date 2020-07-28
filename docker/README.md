# Docker instructions for solc-verify

This Dockerfile allows to quickly try solc-verify using [Docker](https://docs.docker.com/).

The Dockerfile can be built with one of the following commands:

## Build from Open Source

The following Dockerfile will clone the latest solitidy version from GitHub. This is useful if you only have the solc-verify Dockerfile, but not the sourcecode itself.

```
docker build -t solc-verify -f Dockerfile-solcverify.src .
```

## Build locally

Building locally is useful, if you are working on a local copy of solc-verify or in a CI pipeline.

```
# go back to solidity root folder
cd ..

# build docker image
docker build -t solc-verify -f docker/Dockerfile-solcverify-local.src .
```

# Verify Contracts

## Use `runsv.sh` script

The included `runsv.sh` script can be used run the containerized solc-verify on programs residing on the host:
```
./runsv.sh Contract.sol [FLAGS]
```

> Example: `./runsv.sh ../test/solc-verify/examples/Annotations.sol`

## Directly use docker commands to verify contracts

### Validate sample contract

Verify that the docker image is functioning correctly by validating one of the built in contracts. The following command instantiates the SOLC-Verify image, verifies a contract and then shuts down again:

```bash
PATH_TO_SAMPLE_CONTRACT=/solidity/test/solc-verify/examples/Annotations.sol
docker run --rm solc-verify:latest $PATH_TO_SAMPLE_CONTRACT
```

The command should return following messages:

```
C::add: OK
C::[implicit_constructor]: OK
C::[receive_ether_selfdestruct]: OK
Use --show-warnings to see 1 warning.
No errors found.
```

### Validate your contracts

To validate your own contracts place them in a directory that can be mounted into a running container of SOLC-Verify. All you need is the absolute path to the location, where the contracts are located. The following command will mount the contracts located in `$PATH_TO_CONTRACTS` into a running docker container of SOLC-verify and will then validate the contract with the name `$CONTRACT_NAME`.


```bash
PATH_TO_CONTRACTS= # path where the contracts are located on the machine running the docker image
CONTRACT_NAME= # name of the contract to be validated

docker run --rm -v $PATH_TO_CONTRACTS:/contracts solc-verify:latest /contracts/$CONTRACT_NAME
```

For more information on running solc-verify and examples, see [SOLC-VERIFY-README.md](../SOLC-VERIFY-README.md).
