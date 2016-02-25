; Format of GPT Partition Table Header

	struc	gpth
.Signature			resb	8
.Revision			resb	4
.HeaderSize			resb	4
.HeaderCRC32			resb	4
.Reserved			resb	4
.MyLBA				resb	8
.AlternateLBA			resb	8
.FirstUsableLBA			resb	8
.LastUsableLBA			resb	8
.DiskGUID			resb	16
.PartitionEntryLBA		resb	8
.NumberOfPartitionEntries	resb	4
.SizeOfPartitionEntry		resb	4
.PartitionEntryArrayCRC32	resb	4
	endstruc

; Format of GUID Partition Entry Array

	struc	gpte
.PartitionTypeGUID	resb	16
.UniquePartitionGUID	resb	16
.StartingLBA		resb	8
.EndingLBA		resb	8
.Attributes		resb	8
.PartitionName		resb	72
	endstruc
