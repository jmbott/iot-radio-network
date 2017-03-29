# RF Main Controller to Node Communcation Schema

|  Number | Call/Response | Description | Start Byte | Length | Function | Meter ID | Additional Information | Random | Checksum | End Byte |
|  ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ |
|  1 | Call | Beacon | ** | [2B] | 0x01 | [2B] | empty | [1B] | [1B] | ## |
|   | Response |  |  |  |  |  | same as the call  |  |  |  |
|  2 | Call | Read Meter | ** | [2B] | 0x02 | [2B] | empty | [1B] | [1B] | ## |
|   | Response |  | ** | [2B] |  | [2B] | [12B-Voltage, Current, Power, Power Factor, Energy Consumed] (2 messages) | [1B] | [1B] | ## |
|  3 | Call | Switch Relay | ** | [2B] | 0x03 | [2B] | [1B-Status:0x00(off),0x01(on)] | [1B] | [1B] | ## |
|   | Response |  | ** | [2B] |  | [2B] | [1B-ValidatedCommunication:0x01(yes), 0x02(no)] | [1B] | [1B] | ## |
|  4 | Call | Set Tariff | ** | [2B] | 0x04 | [2B] | [4B-Timestamp#1] [4B-Price#1] [4B-Timestamp#2] [4B-Price#2] [4B-GeneratedTimeStamp] [4B-ActivateTimeStamp] (3 messages) | [1B] | [1B] | ## |
|   | Response |  | ** | [2B] |  | [2B] | [1B-ValidatedCommunication:0x01(yes), 0x02(no), 0x03(expired), 0x04(newer availible)] | [1B] | [1B] | ## |
|  5 | Call | Check Credit | ** | [2B] | 0x05 | [2B] | empty | [1B] | [1B] | ## |
|   | Response |  | ** | [2B] |  | [2B] | [2B-Current Credit] | [1B] | [1B] | ## |
|  6 | Call | Recharge Meter | ** | [2B] | 0x06 | [2B] | [2B-Credit to Add] | [1B] | [1B] | ## |
|   | Response |  | ** | [2B] |  | [2B] | [1B-ValidatedCommunication:0x01(yes),0x02(no)] | [1B] | [1B] | ## |
|  7 | Call | Time Sync | ** | [2B] | 0x07 | [2B] | [4B-CurrentTimeStamp] | [1B] | [1B] | ## |
|   | Response |  | ** | [2B] |  | [2B] | [1B-ValidatedCommunication:0x01(yes),0x02(no)] | [1B] | [1B] | ## |

#### 8 byte max in Additional Information per message
#### Length specifies number of messages by specifying number of bytes
