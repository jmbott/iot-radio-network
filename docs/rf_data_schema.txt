# RF Main Controller to Node Communcation Schema


| Number        | 1                                                                         |
| Description   | Beacon                                                                    |
| ----------------------------------------------------------------------------------------- |
| Call/Response | Call                                                                      |
| Start Byte    | AA AA AA                                                                  |
| Length        | 0x04                                                                      |
| Version       | 0x01                                                                      |
| Function      | 0x01                                                                      |
| Meter ID      | [1B]                                                                      |
| Data          | [4B] (Timestamp, not implemented)                                         |
| UUID          | [4B]                                                                      |
| Checksum      | [1B]                                                                      |
| End Byte      | FF FF FF                                                                  |
| ----------------------------------------------------------------------------------------- |
| Note          | Response Echos Call                                                       |
| ----------------------------------------------------------------------------------------- |

| Number        | 2                                                                         |
| Description   | Read Meter                                                                |
| ----------------------------------------------------------------------------------------- |
| Call/Response | Call                                                                      |
| Start Byte    | AA AA AA                                                                  |
| Length        | 0x00                                                                      |
| Version       | 0x01                                                                      |
| Function      | 0x02                                                                      |
| Meter ID      | [1B]                                                                      |
| Data          | (Empty)                                                                   |
| UUID          | [4B]                                                                      |
| Checksum      | [1B]                                                                      |
| End Byte      | FF FF FF                                                                  |
| ----------------------------------------------------------------------------------------- |
| Call/Response | Response                                                                  |
| Start Byte    | AA AA AA                                                                  |
| Length        | 0x16                                                                      |
| Version       | 0x01                                                                      |
| Function      | 0x02                                                                      |
| Meter ID      | [1B]                                                                      |
| Data          | [2B Voltage] [2B Current] [2B Frequency] [2B Power] [2B Power Factor]
  [4B Energy] [2B Relay Status] [2B Temperature] [2B Warnings] [2B Coil Flag]               |
| UUID          | [4B]                                                                      |
| Checksum      | [1B]                                                                      |
| End Byte      | FF FF FF                                                                  |
| ----------------------------------------------------------------------------------------- |

| Number        | 3                                                                         |
| Description   | Switch Relay                                                              |
| ----------------------------------------------------------------------------------------- |
| Call/Response | Call                                                                      |
| Start Byte    | AA AA AA                                                                  |
| Length        | 0x01                                                                      |
| Version       | 0x01                                                                      |
| Function      | 0x03                                                                      |
| Meter ID      | [1B]                                                                      |
| Data          | [1B Status:0x00(off),0x01(on)]                                            |
| UUID          | [4B]                                                                      |
| Checksum      | [1B]                                                                      |
| End Byte      | FF FF FF                                                                  |
| ----------------------------------------------------------------------------------------- |
| Call/Response | Response                                                                  |
| Start Byte    | AA AA AA                                                                  |
| Length        | 0x01                                                                      |
| Version       | 0x01                                                                      |
| Function      | 0x03                                                                      |
| Meter ID      | [1B]                                                                      |
| Data          | [1B ValidatedCommunication:0x01(yes),0x02(no)]                            |
| UUID          | [4B]                                                                      |
| Checksum      | [1B]                                                                      |
| End Byte      | FF FF FF                                                                  |
| ----------------------------------------------------------------------------------------- |

| Number        | 4                                                                         |
| Description   | Set Tariff (not implemented)                                              |
| ----------------------------------------------------------------------------------------- |
| Call/Response | Call                                                                      |
| Start Byte    | AA AA AA                                                                  |
| Length        | 0x18                                                                      |
| Version       | 0x01                                                                      |
| Function      | 0x04                                                                      |
| Meter ID      | [1B]                                                                      |
| Data          | [4B-Timestamp#1] [4B-Price#1] [4B-Timestamp#2] [4B-Price#2]
  [4B-GeneratedTimeStamp] [4B-ActivateTimeStamp]                                            |
| UUID          | [4B]                                                                      |
| Checksum      | [1B]                                                                      |
| End Byte      | FF FF FF                                                                  |
| ----------------------------------------------------------------------------------------- |
| Call/Response | Response                                                                  |
| Start Byte    | AA AA AA                                                                  |
| Length        | 0x01                                                                      |
| Version       | 0x01                                                                      |
| Function      | 0x04                                                                      |
| Meter ID      | [1B]                                                                      |
| Data          | [1B ValidatedCommunication:0x01(yes), 0x02(no), 0x03(expired),
  0x04(newer availible)]                                                                    |
| UUID          | [4B]                                                                      |
| Checksum      | [1B]                                                                      |
| End Byte      | FF FF FF                                                                  |
| ----------------------------------------------------------------------------------------- |

| Number        | 5                                                                         |
| Description   | Check Credit (not implemented)                                            |
| ----------------------------------------------------------------------------------------- |
| Call/Response | Call                                                                      |
| Start Byte    | AA AA AA                                                                  |
| Length        | 0x00                                                                      |
| Version       | 0x01                                                                      |
| Function      | 0x05                                                                      |
| Meter ID      | [1B]                                                                      |
| Data          | (Empty)                                                                   |
| UUID          | [4B]                                                                      |
| Checksum      | [1B]                                                                      |
| End Byte      | FF FF FF                                                                  |
| ----------------------------------------------------------------------------------------- |
| Call/Response | Response                                                                  |
| Start Byte    | AA AA AA                                                                  |
| Length        | 0x02                                                                      |
| Version       | 0x01                                                                      |
| Function      | 0x05                                                                      |
| Meter ID      | [1B]                                                                      |
| Data          | [2B Current Credit]                                                       |
| UUID          | [4B]                                                                      |
| Checksum      | [1B]                                                                      |
| End Byte      | FF FF FF                                                                  |
| ----------------------------------------------------------------------------------------- |

| Number        | 6                                                                         |
| Description   | Recharge Meter (not implemented)                                            |
| ----------------------------------------------------------------------------------------- |
| Call/Response | Call                                                                      |
| Start Byte    | AA AA AA                                                                  |
| Length        | 0x14                                                                      |
| Version       | 0x01                                                                      |
| Function      | 0x06                                                                      |
| Meter ID      | [1B]                                                                      |
| Data          | [2B Credit to Add][16B Credit ID]                                         |
| UUID          | [4B]                                                                      |
| Checksum      | [1B]                                                                      |
| End Byte      | FF FF FF                                                                  |
| ----------------------------------------------------------------------------------------- |
| Call/Response | Response                                                                  |
| Start Byte    | AA AA AA                                                                  |
| Length        | 0x01                                                                      |
| Version       | 0x01                                                                      |
| Function      | 0x06                                                                      |
| Meter ID      | [1B]                                                                      |
| Data          | [1B ValidatedCommunication:0x01(yes), 0x02(no)]                           |
| UUID          | [4B]                                                                      |
| Checksum      | [1B]                                                                      |
| End Byte      | FF FF FF                                                                  |
| ----------------------------------------------------------------------------------------- |

| Number        | 7                                                                         |
| Description   | Version Sync (not implemented)                                            |
| ----------------------------------------------------------------------------------------- |
| Call/Response | Call                                                                      |
| Start Byte    | AA AA AA                                                                  |
| Length        | 0x## (currently undetermined)                                             |
| Version       | 0x01                                                                      |
| Function      | 0x07                                                                      |
| Meter ID      | [1B]                                                                      |
| Data          | [1B NumberSupportedVersions(currently_1)] [#B-Commands&Conventions]       |
| UUID          | [4B]                                                                      |
| Checksum      | [1B]                                                                      |
| End Byte      | FF FF FF                                                                  |
| ----------------------------------------------------------------------------------------- |
| Call/Response | Response                                                                  |
| Start Byte    | AA AA AA                                                                  |
| Length        | 0x01                                                                      |
| Version       | 0x01                                                                      |
| Function      | 0x07                                                                      |
| Meter ID      | [1B]                                                                      |
| Data          | [1B ValidatedCommunication:0x01(yes), 0x02(no)]                           |
| UUID          | [4B]                                                                      |
| Checksum      | [1B]                                                                      |
| End Byte      | FF FF FF                                                                  |
| ----------------------------------------------------------------------------------------- |
