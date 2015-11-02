#ifndef _BOOTVID_H_
#define _BOOTVID_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef
VOID
(*INBV_DISPLAY_STRING_FILTER)(
    PUCHAR *Str
    );

typedef
BOOLEAN
(*INBV_RESET_DISPLAY_PARAMETERS)(
    ULONG Cols,
    ULONG Rows
    );

NTKERNELAPI
VOID
NTAPI
InbvNotifyDisplayOwnershipLost(
    INBV_RESET_DISPLAY_PARAMETERS ResetDisplayParameters
    );

NTKERNELAPI
VOID
NTAPI
InbvInstallDisplayStringFilter(
    INBV_DISPLAY_STRING_FILTER DisplayStringFilter
    );

NTKERNELAPI
VOID
NTAPI
InbvAcquireDisplayOwnership(
    VOID
    );

NTKERNELAPI
BOOLEAN
NTAPI
InbvDriverInitialize(
    IN PVOID LoaderBlock,
    IN ULONG Count
    );

NTKERNELAPI
BOOLEAN
NTAPI
InbvResetDisplay(
    );

NTKERNELAPI
VOID
NTAPI
InbvBitBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y
    );

//设定字符串显示区域
NTKERNELAPI
VOID
NTAPI
InbvSolidColorFill(
    ULONG x1,
    ULONG y1,
    ULONG x2,
    ULONG y2,
    ULONG color
    );

//显示字符串
NTKERNELAPI
BOOLEAN
NTAPI
InbvDisplayString(
    PCHAR Str
    );

//更新进度条进度
NTKERNELAPI
VOID
NTAPI
InbvUpdateProgressBar(
    ULONG Percentage
    );

NTKERNELAPI
VOID
NTAPI
InbvSetProgressBarSubset(
    ULONG   Floor,
    ULONG   Ceiling
    );

NTKERNELAPI
VOID
NTAPI
InbvSetBootDriverBehavior(
    PVOID LoaderBlock
    );

NTKERNELAPI
VOID
NTAPI
InbvIndicateProgress(
    VOID
    );

NTKERNELAPI
VOID
NTAPI
InbvSaveProgressIndicatorCount(
    VOID
    );

NTKERNELAPI
VOID
NTAPI
InbvSetProgressBarCoordinates(
    ULONG x,
    ULONG y
    );

NTKERNELAPI
VOID
NTAPI
InbvEnableBootDriver(
    BOOLEAN bEnable
    );

NTKERNELAPI
BOOLEAN
NTAPI
InbvEnableDisplayString(
    BOOLEAN bEnable
    );

NTKERNELAPI
BOOLEAN
NTAPI
InbvIsBootDriverInstalled(
    VOID
    );

NTKERNELAPI
PUCHAR
NTAPI
InbvGetResourceAddress(
    IN ULONG ResourceNumber
    );

NTKERNELAPI
VOID
NTAPI
InbvBufferToScreenBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y,
    ULONG width,
    ULONG height,
    ULONG lDelta
    );

NTKERNELAPI
VOID
NTAPI
InbvScreenToBufferBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y,
    ULONG width,
    ULONG height,
    ULONG lDelta
    );

NTKERNELAPI
BOOLEAN
NTAPI
InbvTestLock(
    VOID
    );

NTKERNELAPI
VOID
NTAPI
InbvAcquireLock(
    VOID
    );

NTKERNELAPI
VOID
NTAPI
InbvReleaseLock(
    VOID
    );

NTKERNELAPI
BOOLEAN
NTAPI
InbvCheckDisplayOwnership(
    VOID
    );

NTKERNELAPI
VOID
NTAPI
InbvSetScrollRegion(
    ULONG x1,
    ULONG y1,
    ULONG x2,
    ULONG y2
    );

NTKERNELAPI
ULONG
NTAPI
InbvSetTextColor(
    ULONG Color
    );
//#endif

NTKERNELAPI
VOID
NTAPI
InbvSetDisplayOwnership(
    BOOLEAN DisplayOwned
    );

#ifdef __cplusplus
}
#endif

#endif
