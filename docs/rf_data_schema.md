# RF Main Controller to Node Communcation Schema

|  Number | Call/Response | Description | Start Byte | Length | Version | Function | Meter ID | Additional Information | UUID | Checksum | End Byte |
|  ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ |
|  1 | Call | Beacon | *** | [2B] | 0x01 | 0x01 | [4B] | empty | [16B] | [1B] | ### |
|   | Response |  |  |  |  |  |  | same as the call  |  |  |  |
|  2 | Call | Read Meter | *** | [2B] | 0x01 | 0x02 | [4B] | empty | [16B] | [1B] | ### |
|   | Response |  | *** | [2B] |  |  | [4B] | [12B-Voltage, Current, Power, Power Factor, Energy Consumed] | [16B] | [1B] | ### |
|  3 | Call | Switch Relay | *** | [2B] | 0x01 | 0x03 | [4B] | [1B-Status:0x00(off),0x01(on)] | [16B] | [1B] | ### |
|   | Response |  | *** | [2B] |  |  | [4B] | [1B-ValidatedCommunication:0x01(yes),0x02(no)] | [16B] | [1B] | ### |
|  4 | Call | Set Tariff | *** | [2B] | 0x01 | 0x04 | [4B] | [4B-Timestamp#1] [4B-Price#1] [4B-Timestamp#2] [4B-Price#2] [4B-GeneratedTimeStamp] [4B-ActivateTimeStamp] | [16B] | [1B] | ### |
|   | Response |  | *** | [2B] |  |  | [4B] | [1B-ValidatedCommunication:0x01(yes),0x02(no),0x03(expired),0x04(newer availible)] | [16B] | [1B] | ### |
|  5 | Call | Check Credit | *** | [2B] | 0x01 | 0x05 | [4B] | empty | [16B] | [1B] | ### |
|   | Response |  | *** | [2B] |  |  | [4B] | [2B-Current Credit] | [16B] | [1B] | ### |
|  6 | Call | Recharge Meter | *** | [2B] | 0x01 | 0x06 | [4B] | [2B-Credit to Add] | [16B] | [1B] | ### |
|   | Response |  | *** | [2B] |  |  | [4B] | [1B-ValidatedCommunication:0x01(yes),0x02(no)] | [16B] | [1B] | ### |
|  7 | Call | Version Sync | *** | [2B] | 0x01 | 0x07 | [4B] | [1B-NumberSupportedVersions(currently_1)] [#B-Commands&Conventions] | [16B] | [1B] | ### |
|   | Response |  | *** | [2B] |  |  | [4B] | [1B-ValidatedCommunication:0x01(yes),0x02(no)] | [16B] | [1B] | ### |
|  8 | Call | Time Sync | *** | [2B] | 0x01 | 0x08 | [4B] | [4B-CurrentTimeStamp] | [16B] | [1B] | ### |
|   | Response |  | *** | [2B] |  |  | [4B] | [1B-ValidatedCommunication:0x01(yes),0x02(no)] | [16B] | [1B] | ### |
