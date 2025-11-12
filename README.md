# app-nem

NEM wallet application for Ledger devices.

See the [Ledger's documentation](https://developers.ledger.com) to get started.

## Install the application on your Ledger device

1. Modify `source` path for yourself in `load.sh`.
2. Run `load.sh` file to make and install app

```console
./load.sh
```

## Test

1. Setup [Python tools](https://github.com/LedgerHQ/blue-loader-python) for Ledger Nano S.
2. Run the test cases on test directory.

## Permissions

You have to give permissions to connect your Ledger device. See `specs` directory for more information.
