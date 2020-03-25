# Docker instructions for solc-verify

This Dockerfile allows to quickly try solc-verify using [Docker](https://docs.docker.com/).

The Dockerfile can be built with the following command:
```
docker build -t solc-verify -f Dockerfile-solcverify.src .
```

The included `runsv.sh` script can be used run the containerized solc-verify on programs residing on the host:
```
./runsv.sh Contract.sol [FLAGS]
```

For more information on running solc-verify and examples, see [SOLC-VERIFY-README.md](../SOLC-VERIFY-README.md).
