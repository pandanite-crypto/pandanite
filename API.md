# API Guide
General notes:
All transaction amounts, balances, and fees are represented in leaves (1 leaf = 1/10,000 PDN).
All wallet addresses, keys, and block hashes are represented as hexadecimal strings.

#### `GET` /ledger?wallet={string:walletAddress}
Returns the total balance of the wallet in leaves 

Example request:

```
curl http://54.189.82.240:3000/ledger?wallet=0095557B94A368FE2529D3EB33E6BF1276D175D27A4E876249
```

Example response:
```json
{"balance":1575762342}
```


## `GET` /block_count
Check current block count of chain

Example request:
```
curl http://54.189.82.240:3000/block_count
```

Example response:
```
47853
```

## `GET` /block?blockId={int:blockID}
Get data for block. Returns `{"error":"Invalid Block"}` if the block does not exist.

Example request:
```
curl http://54.189.82.240:3000/block?blockId=2
```

Example response:
```json
{
  "difficulty": 16,
  "id": 2,
  "lastBlockHash": "0840EF092D16B7D2D31B6F8CBB855ACF36D73F5778A430B0CEDB93A6E33AF750",
  "merkleRoot": "B5AE82BE0642683888036206CA0F936AA281E755A8425C659F798F46A45699BD",
  "nonce": "D62F3BC7CA5DF50712A823D4002F2F2B0F480E7A804721DF4B31303B474644A2",
  "timestamp": "1644789258",
  "transactions": [
    {
      "amount": 500000,
      "fee": 0,
      "from": "",
      "timestamp": "1644789258",
      "to": "0095557B94A368FE2529D3EB33E6BF1276D175D27A4E876249"
    }
  ]
}

```

## `GET` /create_wallet
Returns a new public key, private key, wallet address. 

NOTE: keypairs generated on untrusted nodes are not secure. Only use this if you are running your own node behind a firewall.

Example request:
```
curl http://54.189.82.240:3000/create_wallet
```

Example response:
```json
{
  "privateKey": "581E2584D2979E0986FC42256588DDDE6CDF9D1A3526E3006F127557DF14EE4DFBCBAE1A08997F3B140A927505255647D4856639971EF879AAEB991BF11C98BE",
  "publicKey": "0010F5BDAA6DEC4539E388C615C76B78F9A8ECD1F71C8EAAA92120329B2B41E5",
  "wallet": "000B8FBE2F3B8C5C579EE1F11C64ABDB4EC020BF5ACDC45718"
}
```

## `GET` /create_transaction
Creates a new signed transaction given privateKey, publicKey, to, amount, fee and a nonce. The nonce can be any random 64bit number.

NOTE: transactions generated on untrusted nodes may leak private keys. Only use this if you are running your own node behind a firewall.

Example request:
```
curl -X POST -H "Content-Type: application/json" -d '{
   "privateKey": "48DF8620FC368E5D5D38EAA6CF075196861565879B524F5927237B2C085FC45F6B5A635A11B1EAB863C27DFFCB167886C85AC11E882789FC4D110C9904FE14E3",
   "publicKey": "3B870B3692B0FC4A93C0067189719D7941263E7F39738111E6D7B87CFC1FDF3A",
   "to": "006FD6A3E7EE4B6F6556502224E6C1FC7232BD449314E7A124",
   "amount": 1,
   "fee": 1,
   "nonce": 0
}' http://localhost:3000/create_transaction

```

Example response:
```json
{
  "amount": 1,
  "fee": 1,
  "from": "004AE69674A9747B462D348DB7188EF284A1157641335B2D1B",
  "signature": "CF96C47A81A77CCC4916BD5BBD31FB1229988459A63FAC66B7E9463A17FFC0C88C607BB6F7979E7B1D60B19764BED229684521CEB3DC5E334FB7C8663E49C00F",
  "signingKey": "3B870B3692B0FC4A93C0067189719D7941263E7F39738111E6D7B87CFC1FDF3A",
  "timestamp": "0",
  "to": "006FD6A3E7EE4B6F6556502224E6C1FC7232BD449314E7A124"
}
```

## `POST` /add_transaction_json 
Submit one or more transactions in JSON format
Example request:
```
curl -X POST -H "Content-Type: application/json" -d  '[
  {
    "amount": 1,
    "fee": 1,
    "from": "004AE69674A9747B462D348DB7188EF284A1157641335B2D1B",
    "signature": "CF96C47A81A77CCC4916BD5BBD31FB1229988459A63FAC66B7E9463A17FFC0C88C607BB6F7979E7B1D60B19764BED229684521CEB3DC5E334FB7C8663E49C00F",
    "signingKey": "3B870B3692B0FC4A93C0067189719D7941263E7F39738111E6D7B87CFC1FDF3A",
    "timestamp": "0",
    "to": "006FD6A3E7EE4B6F6556502224E6C1FC7232BD449314E7A124"
  }
]
' http://localhost:3000/add_transaction_json
```

Example response:
```json
[{"status":"SENDER_DOES_NOT_EXIST", "txid": "06797D19893C8989CE1F72A828DBA8FBB98FCE56265674D4575831AA129ADEE5"}]
```

Status may be any of the following strings:
* `BALANCE_TOO_LOW` : The sender does not have a sufficient balance
* `SENDER_DOES_NOTE_EXIST`: The sender does not exist
* `EXPIRED_TRANSACTION`: The transaction has already been used
* `ALREADY_IN_QUEUE`: The transaction has alredy been submitted
* `QUEUE_FULL`: The node has reached max capacity of transactions


## `POST` /verify_transaction
Get status of one or more transactions in JSON format
Example request:
```
curl -X POST -H "Content-Type: application/json" -d  '[{"txid": "4727299C12A54980B4E49584F358422AB10CA3B77E82F509E6FEB0F6614E2F32"}]' http://localhost:3000/verify_transaction
```

Example response:
```json
[{"status":"IN_CHAIN", "blockId": 1}]
```

Status may be any of the following strings:
* `IN_CHAIN` : The transaction is written to the chain
* `NOT_IN_CHAIN`: The transaction has not been written to the chain

If the transaction is in the chain then the blockId will specify the ID of the block it was written to.

