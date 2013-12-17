/** @file

  OHCI transfer scheduling routines.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ohci.h"


/**
  Create helper QTD/QH for the OHCI device.

  @param  Ohc                   The OHCI device.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource for helper QTD/QH.
  @retval EFI_SUCCESS           Helper QH/QTD are created.

**/
EFI_STATUS
OhcCreateHelpQ (
  IN USB2_HC_DEV          *Ohc
  )
{
  USB_ENDPOINT            Ep;
  OHC_QH                  *Qh;
  QH_HW                   *QhHw;
  OHC_QTD                 *Qtd;
  EFI_PHYSICAL_ADDRESS    PciAddr;

  //
  // Create an inactive Qtd to terminate the short packet read.
  //
  Qtd = OhcCreateQtd (Ohc, NULL, NULL, 0, QTD_PID_INPUT, 0, 64);

  if (Qtd == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Qtd->QtdHw.Status   = QTD_STAT_HALTED;
  Ohc->ShortReadStop  = Qtd;

  //
  // Create a QH to act as the OHC reclamation header.
  // Set the header to loopback to itself.
  //
  Ep.DevAddr    = 0;
  Ep.EpAddr     = 1;
  Ep.Direction  = EfiUsbDataIn;
  Ep.DevSpeed   = EFI_USB_SPEED_HIGH;
  Ep.MaxPacket  = 64;
  Ep.HubAddr    = 0;
  Ep.HubPort    = 0;
  Ep.Toggle     = 0;
  Ep.Type       = OHC_BULK_TRANSFER;
  Ep.PollRate   = 1;

  Qh            = OhcCreateQh (Ohc, &Ep);

  if (Qh == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PciAddr           = UsbHcGetPciAddressForHostMem (Ohc->MemPool, Qh, sizeof (OHC_QH));
  QhHw              = &Qh->QhHw;
  QhHw->HorizonLink = QH_LINK (PciAddr + OFFSET_OF(OHC_QH, QhHw), OHC_TYPE_QH, FALSE);
  QhHw->Status      = QTD_STAT_HALTED;
  QhHw->ReclaimHead = 1;
  Qh->NextQh        = Qh;
  Ohc->ReclaimHead  = Qh;

  //
  // Create a dummy QH to act as the terminator for periodical schedule
  //
  Ep.EpAddr   = 2;
  Ep.Type     = OHC_INT_TRANSFER_SYNC;

  Qh          = OhcCreateQh (Ohc, &Ep);

  if (Qh == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Qh->QhHw.Status = QTD_STAT_HALTED;
  Ohc->PeriodOne  = Qh;

  return EFI_SUCCESS;
}


/**
  Initialize the schedule data structure such as frame list.

  @param  Ohc                   The OHCI device to init schedule data.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource to init schedule data.
  @retval EFI_SUCCESS           The schedule data is initialized.

**/
EFI_STATUS
OhcInitSched (
  IN USB2_HC_DEV          *Ohc
  )
{
  EFI_STATUS            Status;
#if 0
  EFI_PCI_IO_PROTOCOL   *PciIo;
  VOID                  *Buf;
  EFI_PHYSICAL_ADDRESS  PhyAddr;
  VOID                  *Map;
  UINTN                 Pages;
  UINTN                 Bytes;
  UINTN                 Index;
  EFI_PHYSICAL_ADDRESS  PciAddr;
#endif

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  Status = EFI_SUCCESS;

#if 0
  //
  // First initialize the periodical schedule data:
  // 1. Allocate and map the memory for the frame list
  // 2. Create the help QTD/QH
  // 3. Initialize the frame entries
  // 4. Set the frame list register
  //
  PciIo = Ohc->PciIo;

  Bytes = 4096;
  Pages = EFI_SIZE_TO_PAGES (Bytes);

  Status = PciIo->AllocateBuffer (
                    PciIo,
                    AllocateAnyPages,
                    EfiBootServicesData,
                    Pages,
                    &Buf,
                    0
                    );

  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = PciIo->Map (
                    PciIo,
                    EfiPciIoOperationBusMasterCommonBuffer,
                    Buf,
                    &Bytes,
                    &PhyAddr,
                    &Map
                    );

  if (EFI_ERROR (Status) || (Bytes != 4096)) {
    PciIo->FreeBuffer (PciIo, Pages, Buf);
    return EFI_OUT_OF_RESOURCES;
  }

  Ohc->PeriodFrame      = Buf;
  Ohc->PeriodFrameMap   = Map;

#if 0
  //
  // Program the FRAMELISTBASE register with the low 32 bit addr
  //
  OhcWriteOpReg (Ohc, OHC_FRAME_BASE_OFFSET, OHC_LOW_32BIT (PhyAddr));
  //
  // Program the CTRLDSSEGMENT register with the high 32 bit addr
  //
  OhcWriteOpReg (Ohc, OHC_CTRLDSSEG_OFFSET, OHC_HIGH_32BIT (PhyAddr));
#endif

  //
  // Init memory pool management then create the helper
  // QTD/QH. If failed, previously allocated resources
  // will be freed by OhcFreeSched
  //
#if 0
  Ohc->MemPool = UsbHcInitMemPool (
                   PciIo,
                   OHC_BIT_IS_SET (Ohc->HcCapParams, HCCP_64BIT),
                   OHC_HIGH_32BIT (PhyAddr)
                   );
#endif

  if (Ohc->MemPool == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit1;
  }

  Status = OhcCreateHelpQ (Ohc);

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Initialize the frame list entries then set the registers
  //
  Ohc->PeriodFrameHost      = AllocateZeroPool (OHC_FRAME_LEN * sizeof (UINTN));
  if (Ohc->PeriodFrameHost == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  PciAddr  = UsbHcGetPciAddressForHostMem (Ohc->MemPool, Ohc->PeriodOne, sizeof (OHC_QH));

  for (Index = 0; Index < OHC_FRAME_LEN; Index++) {
    //
    // Store the pci bus address of the QH in period frame list which will be accessed by pci bus master.
    //
    ((UINT32 *)(Ohc->PeriodFrame))[Index] = QH_LINK (PciAddr, OHC_TYPE_QH, FALSE);
    //
    // Store the host address of the QH in period frame list which will be accessed by host.
    //
    ((UINTN *)(Ohc->PeriodFrameHost))[Index] = (UINTN)Ohc->PeriodOne;
  }

  //
  // Second initialize the asynchronous schedule:
  // Only need to set the AsynListAddr register to
  // the reclamation header
  //
  PciAddr = UsbHcGetPciAddressForHostMem (Ohc->MemPool, Ohc->ReclaimHead, sizeof (OHC_QH));
#if 0
  OhcWriteOpReg (Ohc, OHC_ASYNC_HEAD_OFFSET, OHC_LOW_32BIT (PciAddr));
#endif
  return EFI_SUCCESS;

ErrorExit:
  if (Ohc->PeriodOne != NULL) {
    UsbHcFreeMem (Ohc->MemPool, Ohc->PeriodOne, sizeof (OHC_QH));
    Ohc->PeriodOne = NULL;
  }

  if (Ohc->ReclaimHead != NULL) {
    UsbHcFreeMem (Ohc->MemPool, Ohc->ReclaimHead, sizeof (OHC_QH));
    Ohc->ReclaimHead = NULL;
  }

  if (Ohc->ShortReadStop != NULL) {
    UsbHcFreeMem (Ohc->MemPool, Ohc->ShortReadStop, sizeof (OHC_QTD));
    Ohc->ShortReadStop = NULL;
  }

ErrorExit1:
  PciIo->FreeBuffer (PciIo, Pages, Buf);
  PciIo->Unmap (PciIo, Map);
#endif

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave with (%r)\n", Status));
  return Status;
}


/**
  Free the schedule data. It may be partially initialized.

  @param  Ohc                   The OHCI device.

**/
VOID
OhcFreeSched (
  IN USB2_HC_DEV          *Ohc
  )
{
  EFI_PCI_IO_PROTOCOL     *PciIo;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

#if 0
  OhcWriteOpReg (Ohc, OHC_FRAME_BASE_OFFSET, 0);
  OhcWriteOpReg (Ohc, OHC_ASYNC_HEAD_OFFSET, 0);
#endif

  if (Ohc->PeriodOne != NULL) {
    UsbHcFreeMem (Ohc->MemPool, Ohc->PeriodOne, sizeof (OHC_QH));
    Ohc->PeriodOne = NULL;
  }

  if (Ohc->ReclaimHead != NULL) {
    UsbHcFreeMem (Ohc->MemPool, Ohc->ReclaimHead, sizeof (OHC_QH));
    Ohc->ReclaimHead = NULL;
  }

  if (Ohc->ShortReadStop != NULL) {
    UsbHcFreeMem (Ohc->MemPool, Ohc->ShortReadStop, sizeof (OHC_QTD));
    Ohc->ShortReadStop = NULL;
  }

  if (Ohc->MemPool != NULL) {
    UsbHcFreeMemPool (Ohc->MemPool);
    Ohc->MemPool = NULL;
  }

  if (Ohc->PeriodFrame != NULL) {
    PciIo = Ohc->PciIo;
    ASSERT (PciIo != NULL);

    PciIo->Unmap (PciIo, Ohc->PeriodFrameMap);

    PciIo->FreeBuffer (
             PciIo,
             EFI_SIZE_TO_PAGES (EFI_PAGE_SIZE),
             Ohc->PeriodFrame
             );

    Ohc->PeriodFrame = NULL;
  }

  if (Ohc->PeriodFrameHost != NULL) {
    FreePool (Ohc->PeriodFrameHost);
    Ohc->PeriodFrameHost = NULL;
  }
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave\n"));
}


/**
  Link the queue head to the asynchronous schedule list.
  UEFI only supports one CTRL/BULK transfer at a time
  due to its interfaces. This simplifies the AsynList
  management: A reclamation header is always linked to
  the AsyncListAddr, the only active QH is appended to it.

  @param  Ohc                   The OHCI device.
  @param  Qh                    The queue head to link.

**/
VOID
OhcLinkQhToAsync (
  IN USB2_HC_DEV          *Ohc,
  IN OHC_QH               *Qh
  )
{
  OHC_QH                  *Head;
  EFI_PHYSICAL_ADDRESS    PciAddr;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  //
  // Append the queue head after the reclaim header, then
  // fix the hardware visiable parts (OHCI R1.0 page 72).
  // ReclaimHead is always linked to the OHCI's AsynListAddr.
  //
  Head                    = Ohc->ReclaimHead;

  Qh->NextQh              = Head->NextQh;
  Head->NextQh            = Qh;

  PciAddr = UsbHcGetPciAddressForHostMem (Ohc->MemPool, Qh->NextQh, sizeof (OHC_QH));
  Qh->QhHw.HorizonLink    = QH_LINK (PciAddr, OHC_TYPE_QH, FALSE);
  PciAddr = UsbHcGetPciAddressForHostMem (Ohc->MemPool, Head->NextQh, sizeof (OHC_QH));
  Head->QhHw.HorizonLink  = QH_LINK (PciAddr, OHC_TYPE_QH, FALSE);

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave\n"));
}


/**
  Unlink a queue head from the asynchronous schedule list.
  Need to synchronize with hardware.

  @param  Ohc                   The OHCI device.
  @param  Qh                    The queue head to unlink.

**/
VOID
OhcUnlinkQhFromAsync (
  IN USB2_HC_DEV          *Ohc,
  IN OHC_QH               *Qh
  )
{
  OHC_QH                  *Head;
  EFI_STATUS              Status;
  EFI_PHYSICAL_ADDRESS    PciAddr;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  ASSERT (Ohc->ReclaimHead->NextQh == Qh);

  //
  // Remove the QH from reclamation head, then update the hardware
  // visiable part: Only need to loopback the ReclaimHead. The Qh
  // is pointing to ReclaimHead (which is staill in the list).
  //
  Head                    = Ohc->ReclaimHead;

  Head->NextQh            = Qh->NextQh;
  Qh->NextQh              = NULL;

  PciAddr = UsbHcGetPciAddressForHostMem (Ohc->MemPool, Head->NextQh, sizeof (OHC_QH));
  Head->QhHw.HorizonLink  = QH_LINK (PciAddr, OHC_TYPE_QH, FALSE);

  //
  // Set and wait the door bell to synchronize with the hardware
  //
  Status = OhcSetAndWaitDoorBell (Ohc, OHC_GENERIC_TIMEOUT);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": Failed to synchronize with doorbell\n"));
  }

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave\n"));
}


/**
  Link a queue head for interrupt transfer to the periodic
  schedule frame list. This code is very much the same as
  that in UHCI.

  @param  Ohc                   The OHCI device.
  @param  Qh                    The queue head to link.

**/
VOID
OhcLinkQhToPeriod (
  IN USB2_HC_DEV          *Ohc,
  IN OHC_QH               *Qh
  )
{
#if 0
  UINTN                   Index;
  OHC_QH                  *Prev;
  OHC_QH                  *Next;
  EFI_PHYSICAL_ADDRESS    PciAddr;
#endif

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

#if 0
  for (Index = 0; Index < OHC_FRAME_LEN; Index += Qh->Interval) {
    //
    // First QH can't be NULL because we always keep PeriodOne
    // heads on the frame list
    //
    ASSERT (!OHC_LINK_TERMINATED (((UINT32*)Ohc->PeriodFrame)[Index]));
    Next  = (OHC_QH*)((UINTN*)Ohc->PeriodFrameHost)[Index];
    Prev  = NULL;

    //
    // Now, insert the queue head (Qh) into this frame:
    // 1. Find a queue head with the same poll interval, just insert
    //    Qh after this queue head, then we are done.
    //
    // 2. Find the position to insert the queue head into:
    //      Previous head's interval is bigger than Qh's
    //      Next head's interval is less than Qh's
    //    Then, insert the Qh between then
    //
    while (Next->Interval > Qh->Interval) {
      Prev  = Next;
      Next  = Next->NextQh;
    }

    ASSERT (Next != NULL);

    //
    // The entry may have been linked into the frame by early insertation.
    // For example: if insert a Qh with Qh.Interval == 4, and there is a Qh
    // with Qh.Interval == 8 on the frame. If so, we are done with this frame.
    // It isn't necessary to compare all the QH with the same interval to
    // Qh. This is because if there is other QH with the same interval, Qh
    // should has been inserted after that at Frames[0] and at Frames[0] it is
    // impossible for (Next == Qh)
    //
    if (Next == Qh) {
      continue;
    }

    if (Next->Interval == Qh->Interval) {
      //
      // If there is a QH with the same interval, it locates at
      // Frames[0], and we can simply insert it after this QH. We
      // are all done.
      //
      ASSERT ((Index == 0) && (Qh->NextQh == NULL));

      Prev                    = Next;
      Next                    = Next->NextQh;

      Qh->NextQh              = Next;
      Prev->NextQh            = Qh;

      Qh->QhHw.HorizonLink    = Prev->QhHw.HorizonLink;
      PciAddr = UsbHcGetPciAddressForHostMem (Ohc->MemPool, Qh, sizeof (OHC_QH));
      Prev->QhHw.HorizonLink  = QH_LINK (PciAddr, OHC_TYPE_QH, FALSE);
      break;
    }

    //
    // OK, find the right position, insert it in. If Qh's next
    // link has already been set, it is in position. This is
    // guarranted by 2^n polling interval.
    //
    if (Qh->NextQh == NULL) {
      Qh->NextQh              = Next;
      PciAddr = UsbHcGetPciAddressForHostMem (Ohc->MemPool, Next, sizeof (OHC_QH));
      Qh->QhHw.HorizonLink    = QH_LINK (PciAddr, OHC_TYPE_QH, FALSE);
    }

    PciAddr = UsbHcGetPciAddressForHostMem (Ohc->MemPool, Qh, sizeof (OHC_QH));

    if (Prev == NULL) {
      ((UINT32*)Ohc->PeriodFrame)[Index]     = QH_LINK (PciAddr, OHC_TYPE_QH, FALSE);
      ((UINTN*)Ohc->PeriodFrameHost)[Index]  = (UINTN)Qh;
    } else {
      Prev->NextQh            = Qh;
      Prev->QhHw.HorizonLink  = QH_LINK (PciAddr, OHC_TYPE_QH, FALSE);
    }
  }
#endif

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave\n"));
}


/**
  Unlink an interrupt queue head from the periodic
  schedule frame list.

  @param  Ohc                   The OHCI device.
  @param  Qh                    The queue head to unlink.

**/
VOID
OhcUnlinkQhFromPeriod (
  IN USB2_HC_DEV          *Ohc,
  IN OHC_QH               *Qh
  )
{
#if 0
  UINTN                   Index;
  OHC_QH                  *Prev;
  OHC_QH                  *This;
#endif

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave\n"));

#if 0
  for (Index = 0; Index < OHC_FRAME_LEN; Index += Qh->Interval) {
    //
    // Frame link can't be NULL because we always keep PeroidOne
    // on the frame list
    //
    ASSERT (!OHC_LINK_TERMINATED (((UINT32*)Ohc->PeriodFrame)[Index]));
    This  = (OHC_QH*)((UINTN*)Ohc->PeriodFrameHost)[Index];
    Prev  = NULL;

    //
    // Walk through the frame's QH list to find the
    // queue head to remove
    //
    while ((This != NULL) && (This != Qh)) {
      Prev  = This;
      This  = This->NextQh;
    }

    //
    // Qh may have already been unlinked from this frame
    // by early action. See the comments in OhcLinkQhToPeriod.
    //
    if (This == NULL) {
      continue;
    }

    if (Prev == NULL) {
      //
      // Qh is the first entry in the frame
      //
      ((UINT32*)Ohc->PeriodFrame)[Index] = Qh->QhHw.HorizonLink;
      ((UINTN*)Ohc->PeriodFrameHost)[Index] = (UINTN)Qh->NextQh;
    } else {
      Prev->NextQh            = Qh->NextQh;
      Prev->QhHw.HorizonLink  = Qh->QhHw.HorizonLink;
    }
  }
#endif

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave\n"));
}

/**
  Check the URB's execution result and update the URB's
  result accordingly.

  @param  Ohc                   The OHCI device.
  @param  Urb                   The URB to check result.

  @return Whether the result of URB transfer is finialized.

**/
BOOLEAN
OhcCheckUrbResult (
  IN  USB2_HC_DEV         *Ohc,
  IN  URB                 *Urb
  )
{
  LIST_ENTRY              *Entry;
  OHC_QTD                 *Qtd;
  QTD_HW                  *QtdHw;
  UINT8                   State;
  BOOLEAN                 Finished;
  EFI_PHYSICAL_ADDRESS    PciAddr;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  ASSERT ((Ohc != NULL) && (Urb != NULL) && (Urb->Qh != NULL));

  Finished        = TRUE;
  Urb->Completed  = 0;

  Urb->Result     = EFI_USB_NOERROR;

  if (OhcIsHalt (Ohc) || OhcIsSysError (Ohc)) {
    Urb->Result |= EFI_USB_ERR_SYSTEM;
    goto ON_EXIT;
  }

  EFI_LIST_FOR_EACH (Entry, &Urb->Qh->Qtds) {
    Qtd   = EFI_LIST_CONTAINER (Entry, OHC_QTD, QtdList);
    QtdHw = &Qtd->QtdHw;
    State = (UINT8) QtdHw->Status;

    if (OHC_BIT_IS_SET (State, QTD_STAT_HALTED)) {
      //
      // OHCI will halt the queue head when met some error.
      // If it is halted, the result of URB is finialized.
      //
      if ((State & QTD_STAT_ERR_MASK) == 0) {
        Urb->Result |= EFI_USB_ERR_STALL;
      }

      if (OHC_BIT_IS_SET (State, QTD_STAT_BABBLE_ERR)) {
        Urb->Result |= EFI_USB_ERR_BABBLE;
      }

      if (OHC_BIT_IS_SET (State, QTD_STAT_BUFF_ERR)) {
        Urb->Result |= EFI_USB_ERR_BUFFER;
      }

      if (OHC_BIT_IS_SET (State, QTD_STAT_TRANS_ERR) && (QtdHw->ErrCnt == 0)) {
        Urb->Result |= EFI_USB_ERR_TIMEOUT;
      }

      Finished = TRUE;
      goto ON_EXIT;

    } else if (OHC_BIT_IS_SET (State, QTD_STAT_ACTIVE)) {
      //
      // The QTD is still active, no need to check furthur.
      //
      Urb->Result |= EFI_USB_ERR_NOTEXECUTE;

      Finished = FALSE;
      goto ON_EXIT;

    } else {
      //
      // This QTD is finished OK or met short packet read. Update the
      // transfer length if it isn't a setup.
      //
      if (QtdHw->Pid != QTD_PID_SETUP) {
        Urb->Completed += Qtd->DataLen - QtdHw->TotalBytes;
      }

      if ((QtdHw->TotalBytes != 0) && (QtdHw->Pid == QTD_PID_INPUT)) {
        OhcDumpQh (Urb->Qh, "Short packet read", FALSE);

        //
        // Short packet read condition. If it isn't a setup transfer,
        // no need to check furthur: the queue head will halt at the
        // ShortReadStop. If it is a setup transfer, need to check the
        // Status Stage of the setup transfer to get the finial result
        //
        PciAddr = UsbHcGetPciAddressForHostMem (Ohc->MemPool, Ohc->ShortReadStop, sizeof (OHC_QTD));
        if (QtdHw->AltNext == QTD_LINK (PciAddr, FALSE)) {
          DEBUG ((EFI_D_VERBOSE,  __FUNCTION__ ": Short packet read, break\n"));

          Finished = TRUE;
          goto ON_EXIT;
        }

        DEBUG ((EFI_D_VERBOSE,  __FUNCTION__ ": Short packet read, continue\n"));
      }
    }
  }

ON_EXIT:
  //
  // Return the data toggle set by OHCI hardware, bulk and interrupt
  // transfer will use this to initialize the next transaction. For
  // Control transfer, it always start a new data toggle sequence for
  // new transfer.
  //
  // NOTICE: don't move DT update before the loop, otherwise there is
  // a race condition that DT is wrong.
  //
  Urb->DataToggle = (UINT8) Urb->Qh->QhHw.DataToggle;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave\n"));
  return Finished;
}


/**
  Execute the transfer by polling the URB. This is a synchronous operation.

  @param  Ohc               The OHCI device.
  @param  Urb               The URB to execute.
  @param  TimeOut           The time to wait before abort, in millisecond.

  @return EFI_DEVICE_ERROR  The transfer failed due to transfer error.
  @return EFI_TIMEOUT       The transfer failed due to time out.
  @return EFI_SUCCESS       The transfer finished OK.

**/
EFI_STATUS
OhcExecTransfer (
  IN  USB2_HC_DEV         *Ohc,
  IN  URB                 *Urb,
  IN  UINTN               TimeOut
  )
{
  EFI_STATUS              Status;
  UINTN                   Index;
  UINTN                   Loop;
  BOOLEAN                 Finished;
  BOOLEAN                 InfiniteLoop;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  Status       = EFI_SUCCESS;
  Loop         = TimeOut * OHC_1_MILLISECOND;
  Finished     = FALSE;
  InfiniteLoop = FALSE;

  //
  // According to UEFI spec section 16.2.4, If Timeout is 0, then the caller
  // must wait for the function to be completed until EFI_SUCCESS or EFI_DEVICE_ERROR
  // is returned.
  //
  if (TimeOut == 0) {
    InfiniteLoop = TRUE;
  }

  for (Index = 0; InfiniteLoop || (Index < Loop); Index++) {
    Finished = OhcCheckUrbResult (Ohc, Urb);

    if (Finished) {
      break;
    }

    gBS->Stall (OHC_1_MICROSECOND);
  }

  if (!Finished) {
    DEBUG ((EFI_D_ERROR,  __FUNCTION__ ": transfer not finished in %dms\n", (UINT32)TimeOut));
    OhcDumpQh (Urb->Qh, NULL, FALSE);

    Status = EFI_TIMEOUT;

  } else if (Urb->Result != EFI_USB_NOERROR) {
    DEBUG ((EFI_D_ERROR,  __FUNCTION__ ": transfer failed with %x\n", Urb->Result));
    OhcDumpQh (Urb->Qh, NULL, FALSE);

    Status = EFI_DEVICE_ERROR;
  }

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave with (%r)\n", Status));
  return Status;
}


/**
  Delete a single asynchronous interrupt transfer for
  the device and endpoint.

  @param  Ohc                   The OHCI device.
  @param  DevAddr               The address of the target device.
  @param  EpNum                 The endpoint of the target.
  @param  DataToggle            Return the next data toggle to use.

  @retval EFI_SUCCESS           An asynchronous transfer is removed.
  @retval EFI_NOT_FOUND         No transfer for the device is found.

**/
EFI_STATUS
OhciDelAsyncIntTransfer (
  IN  USB2_HC_DEV         *Ohc,
  IN  UINT8               DevAddr,
  IN  UINT8               EpNum,
  OUT UINT8               *DataToggle
  )
{
  LIST_ENTRY              *Entry;
  LIST_ENTRY              *Next;
  URB                     *Urb;
  EFI_USB_DATA_DIRECTION  Direction;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  Direction = (((EpNum & 0x80) != 0) ? EfiUsbDataIn : EfiUsbDataOut);
  EpNum    &= 0x0F;

  EFI_LIST_FOR_EACH_SAFE (Entry, Next, &Ohc->AsyncIntTransfers) {
    Urb = EFI_LIST_CONTAINER (Entry, URB, UrbList);

    if ((Urb->Ep.DevAddr == DevAddr) && (Urb->Ep.EpAddr == EpNum) &&
        (Urb->Ep.Direction == Direction)) {
      //
      // Check the URB status to retrieve the next data toggle
      // from the associated queue head.
      //
      OhcCheckUrbResult (Ohc, Urb);
      *DataToggle = Urb->DataToggle;

      OhcUnlinkQhFromPeriod (Ohc, Urb->Qh);
      RemoveEntryList (&Urb->UrbList);

      gBS->FreePool (Urb->Data);
      OhcFreeUrb (Ohc, Urb);
      DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave (success)\n"));
      return EFI_SUCCESS;
    }
  }

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave (not found)\n"));
  return EFI_NOT_FOUND;
}


/**
  Remove all the asynchronous interrutp transfers.

  @param  Ohc                   The OHCI device.

**/
VOID
OhciDelAllAsyncIntTransfers (
  IN USB2_HC_DEV          *Ohc
  )
{
  LIST_ENTRY              *Entry;
  LIST_ENTRY              *Next;
  URB                     *Urb;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  EFI_LIST_FOR_EACH_SAFE (Entry, Next, &Ohc->AsyncIntTransfers) {
    Urb = EFI_LIST_CONTAINER (Entry, URB, UrbList);

    OhcUnlinkQhFromPeriod (Ohc, Urb->Qh);
    RemoveEntryList (&Urb->UrbList);

    gBS->FreePool (Urb->Data);
    OhcFreeUrb (Ohc, Urb);
  }
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave\n"));
}


/**
  Flush data from PCI controller specific address to mapped system
  memory address.

  @param  Ohc                The OHCI device.
  @param  Urb                The URB to unmap.

  @retval EFI_SUCCESS        Success to flush data to mapped system memory.
  @retval EFI_DEVICE_ERROR   Fail to flush data to mapped system memory.

**/
EFI_STATUS
OhcFlushAsyncIntMap (
  IN  USB2_HC_DEV         *Ohc,
  IN  URB                 *Urb
  )
{
  EFI_STATUS                    Status;
  EFI_PHYSICAL_ADDRESS          PhyAddr;
  EFI_PCI_IO_PROTOCOL_OPERATION MapOp;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  UINTN                         Len;
  VOID                          *Map;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  PciIo = Ohc->PciIo;
  Len   = Urb->DataLen;

  if (Urb->Ep.Direction == EfiUsbDataIn) {
    MapOp = EfiPciIoOperationBusMasterWrite;
  } else {
    MapOp = EfiPciIoOperationBusMasterRead;
  }

  Status = PciIo->Unmap (PciIo, Urb->DataMap);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Urb->DataMap = NULL;

  Status = PciIo->Map (PciIo, MapOp, Urb->Data, &Len, &PhyAddr, &Map);
  if (EFI_ERROR (Status) || (Len != Urb->DataLen)) {
    goto ON_ERROR;
  }

  Urb->DataPhy  = (VOID *) ((UINTN) PhyAddr);
  Urb->DataMap  = Map;
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave with (success)\n"));
  return EFI_SUCCESS;

ON_ERROR:
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave with (device error)\n"));
  return EFI_DEVICE_ERROR;
}


/**
  Update the queue head for next round of asynchronous transfer.

  @param  Ohc                   The OHCI device.
  @param  Urb                   The URB to update.

**/
VOID
OhcUpdateAsyncRequest (
  IN  USB2_HC_DEV         *Ohc,
  IN URB                  *Urb
  )
{
  LIST_ENTRY              *Entry;
  OHC_QTD                 *FirstQtd;
  QH_HW                   *QhHw;
  OHC_QTD                 *Qtd;
  QTD_HW                  *QtdHw;
  UINTN                   Index;
  EFI_PHYSICAL_ADDRESS    PciAddr;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  Qtd = NULL;

  if (Urb->Result == EFI_USB_NOERROR) {
    FirstQtd = NULL;

    EFI_LIST_FOR_EACH (Entry, &Urb->Qh->Qtds) {
      Qtd = EFI_LIST_CONTAINER (Entry, OHC_QTD, QtdList);

      if (FirstQtd == NULL) {
        FirstQtd = Qtd;
      }

      //
      // Update the QTD for next round of transfer. Host control
      // may change dt/Total Bytes to Transfer/C_Page/Cerr/Status/
      // Current Offset. These fields need to be updated. DT isn't
      // used by interrupt transfer. It uses DT in queue head.
      // Current Offset is in Page[0], only need to reset Page[0]
      // to initial data buffer.
      //
      QtdHw             = &Qtd->QtdHw;
      QtdHw->Status     = QTD_STAT_ACTIVE;
      QtdHw->ErrCnt     = QTD_MAX_ERR;
      QtdHw->CurPage    = 0;
      QtdHw->TotalBytes = (UINT32) Qtd->DataLen;
      //
      // calculate physical address by offset.
      //
      PciAddr = (UINTN)Urb->DataPhy + ((UINTN)Qtd->Data - (UINTN)Urb->Data); 
      QtdHw->Page[0]    = OHC_LOW_32BIT (PciAddr);
      QtdHw->PageHigh[0]= OHC_HIGH_32BIT (PciAddr);
    }

    //
    // Update QH for next round of transfer. Host control only
    // touch the fields in transfer overlay area. Only need to
    // zero out the overlay area and set NextQtd to the first
    // QTD. DateToggle bit is left untouched.
    //
    QhHw              = &Urb->Qh->QhHw;
    QhHw->CurQtd      = QTD_LINK (0, TRUE);
    QhHw->AltQtd      = 0;

    QhHw->Status      = 0;
    QhHw->Pid         = 0;
    QhHw->ErrCnt      = 0;
    QhHw->CurPage     = 0;
    QhHw->Ioc         = 0;
    QhHw->TotalBytes  = 0;

    for (Index = 0; Index < 5; Index++) {
      QhHw->Page[Index]     = 0;
      QhHw->PageHigh[Index] = 0;
    }

    PciAddr = UsbHcGetPciAddressForHostMem (Ohc->MemPool, FirstQtd, sizeof (OHC_QTD));
    QhHw->NextQtd = QTD_LINK (PciAddr, FALSE);
  }

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave\n"));
  return ;
}


/**
  Interrupt transfer periodic check handler.

  @param  Event                 Interrupt event.
  @param  Context               Pointer to USB2_HC_DEV.

**/
VOID
EFIAPI
OhcMonitorAsyncRequests (
  IN EFI_EVENT            Event,
  IN VOID                 *Context
  )
{
  USB2_HC_DEV             *Ohc;
  EFI_TPL                 OldTpl;
  LIST_ENTRY              *Entry;
  LIST_ENTRY              *Next;
  BOOLEAN                 Finished;
  UINT8                   *ProcBuf;
  URB                     *Urb;
  EFI_STATUS              Status;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  OldTpl  = gBS->RaiseTPL (OHC_TPL);
  Ohc     = (USB2_HC_DEV *) Context;

  EFI_LIST_FOR_EACH_SAFE (Entry, Next, &Ohc->AsyncIntTransfers) {
    Urb = EFI_LIST_CONTAINER (Entry, URB, UrbList);

    //
    // Check the result of URB execution. If it is still
    // active, check the next one.
    //
    Finished = OhcCheckUrbResult (Ohc, Urb);

    if (!Finished) {
      continue;
    }

    //
    // Flush any PCI posted write transactions from a PCI host
    // bridge to system memory.
    //
    Status = OhcFlushAsyncIntMap (Ohc, Urb);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR,  __FUNCTION__ ": Fail to Flush AsyncInt Mapped Memeory\n"));
    }

    //
    // Allocate a buffer then copy the transferred data for user.
    // If failed to allocate the buffer, update the URB for next
    // round of transfer. Ignore the data of this round.
    //
    ProcBuf = NULL;

    if (Urb->Result == EFI_USB_NOERROR) {
      ASSERT (Urb->Completed <= Urb->DataLen);

      ProcBuf = AllocatePool (Urb->Completed);

      if (ProcBuf == NULL) {
        OhcUpdateAsyncRequest (Ohc, Urb);
        continue;
      }

      CopyMem (ProcBuf, Urb->Data, Urb->Completed);
    }

    OhcUpdateAsyncRequest (Ohc, Urb);

    //
    // Leave error recovery to its related device driver. A
    // common case of the error recovery is to re-submit the
    // interrupt transfer which is linked to the head of the
    // list. This function scans from head to tail. So the
    // re-submitted interrupt transfer's callback function
    // will not be called again in this round. Don't touch this
    // URB after the callback, it may have been removed by the
    // callback.
    //
    if (Urb->Callback != NULL) {
      //
      // Restore the old TPL, USB bus maybe connect device in
      // his callback. Some drivers may has a lower TPL restriction.
      //
      gBS->RestoreTPL (OldTpl);
      (Urb->Callback) (ProcBuf, Urb->Completed, Urb->Context, Urb->Result);
      OldTpl = gBS->RaiseTPL (OHC_TPL);
    }

    if (ProcBuf != NULL) {
      FreePool (ProcBuf);
    }
  }

  gBS->RestoreTPL (OldTpl);
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave\n"));
}
