//位图操作文件
//  [1/26/2012 Administrator]
//  作者：罗澄曦
#ifndef _BITMAP_MGR_H_
#define _BITMAP_MGR_H_

#include <windef.h>

//存储位图的结构
//  按需申请内存
typedef struct _LCXL_BITMAP {
	//位图有多少个单位，以bit为单位
	ULONGLONG	BitmapSize;
	// 每个块占多少ULONG_PTR
	ULONG		regionUlongPtr;
	// 指向bitmap存储空间的指针列表的长度
	ULONG		bufferListLen;
	// 指向bitmap存储空间的指针列表
	PULONG_PTR		*bufferList;
} LCXL_BITMAP, * PLCXL_BITMAP;

//初始化位图结构
NTSTATUS LCXLBitmapInit(
	OUT PLCXL_BITMAP bitmap,	 // 位图句柄指针
	IN ULONGLONG BitmapSize,	 // 位图有多少个单位
	IN ULONG regionBytes	     // 位图粒度，分成N块，一块占多少byte
	);

//设置位图数据
NTSTATUS LCXLBitmapSet(
	IN OUT PLCXL_BITMAP bitmap,	 // 位图句柄指针
	IN ULONGLONG FirstBitmapIndex,// 从位图序号开始，以位为单位
	IN ULONGLONG BitmapNum,		//要写入BitmapNum位的数据
	IN BOOLEAN IsSetTo1			//是否设置为1
	);

//获取位图数据
BOOLEAN LCXLBitmapGet(
	IN PLCXL_BITMAP bitmap,	 // 位图句柄指针
	IN ULONGLONG BitmapIndex // 位图序号，以位为单位
	);

//释放位图结构
VOID LCXLBitmapFina(
	IN OUT PLCXL_BITMAP bitmap	 // 位图句柄指针
	);

//获取位图数据
__inline BOOLEAN BitmapGet(
	IN PBYTE bitmap,		 // 位图数据指针
	IN ULONGLONG BitmapIndex // 位图序号，以位为单位
	)
{
	ULONGLONG BitmapByteIndex;//位图序号，以字节为单位
	BYTE BitmapBitIndex;//位图序号在此字节中的序号

	BitmapByteIndex = BitmapIndex/8;//获取此位图序号在哪个字节上
	BitmapBitIndex = BitmapIndex%8;//获取此位图序号在字节上的位置
	return ((bitmap[BitmapByteIndex]&(1<<BitmapBitIndex))!=0);
}

//设置位图数据
__inline VOID BitmapSet(
	IN OUT PBYTE bitmap,		 // 位图数据指针
	IN ULONGLONG BitmapIndex, // 位图序号，以位为单位
	IN BOOLEAN IsSetTo1			//是否设置为1
	)
{
	ULONGLONG BitmapByteIndex;//位图序号，以字节为单位
	BYTE BitmapBitIndex;//位图序号在此字节中的序号

	BitmapByteIndex = BitmapIndex/8;//获取此位图序号在哪个字节上
	BitmapBitIndex = BitmapIndex%8;//获取此位图序号在字节上的位置
	//如果置为1
	if (IsSetTo1) {
		//设置位图数据，将对应的数据置为1
		bitmap[BitmapByteIndex] |= (BYTE)1<<BitmapBitIndex;
	} else {
		//否则
		bitmap[BitmapByteIndex] &= ~((BYTE)1<<BitmapBitIndex);
	}
}

//寻找空闲的位(即为0的位)的序号
//如果返回的序号为BitmapSize，则表明位图已用完
ULONGLONG LCXLBitmapFindFreeBit(
	IN PLCXL_BITMAP LcxlBitmap,		 // 位图数据指针
	IN ULONGLONG StartOffset //从StartOffset处开始找，实际中它会以StartOffset/sizeof(ULONG_PTR)*8/(sizeof(ULONG_PTR)*8)开始找，也就是按ULONG_PTR对齐，提高速度
	);

//通过Bitmap创建LCXLBitmap位图（LCXL位图的大小为BitmapSize+BitmapOffset）
NTSTATUS LCXLBitmapCreateFromBitmap(
	IN PBYTE bitmap,		 // 位图数据指针
	IN ULONGLONG BitmapSize, // 位图大小，以位为单位
	IN ULONGLONG BitmapOffset,//bitmap位图在LCXLBitmap位图的偏移量
	IN ULONG regionBytes,	     // 位图粒度，分成N块，一块占多少byte
	OUT PLCXL_BITMAP LcxlBitmap//新的LCXL位图
	);

#endif