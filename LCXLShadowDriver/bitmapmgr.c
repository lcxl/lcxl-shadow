// [1/26/2012 Administrator]
// 作者：罗澄曦
#include <ntddk.h>
#include "bitmapmgr.h"

#define INFO_BITMAP_TAG 'BMPT'

//初始化位图结构
NTSTATUS LCXLBitmapInit(
	IN OUT PLCXL_BITMAP bitmap,	 // 位图句柄指针
	IN ULONGLONG BitmapSize,	 // 位图大小，以位bit为单位
	IN ULONG regionBytes	     // 位图粒度，分成N块，一块占多少byte
	)
{
	ULONGLONG BitmapUlongPtrSize;//位图大小，以UlongPtr为单位
	NTSTATUS status;

	PAGED_CODE();

	ASSERT(bitmap!=NULL);
	ASSERT(regionBytes>0);
	//位图大小，以位bit为单位
	bitmap->BitmapSize = BitmapSize;
	//位图粒度，分成N块，一块占多少ULONG_PTR
	bitmap->regionUlongPtr = regionBytes/sizeof(ULONG_PTR)+(regionBytes%sizeof(ULONG_PTR)>0);
	
	//计算位图的ULONG_PTR数量大小，一个ULONG_PTR能存32bit（32位系统）/64bit（64位系统）的数据
	BitmapUlongPtrSize = BitmapSize/(sizeof(ULONG_PTR)<<3)+(BitmapSize%(sizeof(ULONG_PTR)<<3)>0);
	//计算Buffer列表的长度N(将ULONGLONG类型强制转化为ULONG)
	bitmap->bufferListLen = (ULONG)(BitmapUlongPtrSize/bitmap->regionUlongPtr+(BitmapUlongPtrSize%bitmap->regionUlongPtr>0));
	//根据Buffer列表的长度来申请内存来存储位图列表
	bitmap->bufferList = (PULONG_PTR*)ExAllocatePoolWithTag( PagedPool, bitmap->bufferListLen*sizeof(PULONG_PTR), INFO_BITMAP_TAG);
	//如果申请不到内存
	if (bitmap->bufferList != NULL) {
		//将指针全部置零，表示为空
		RtlZeroMemory(bitmap->bufferList, bitmap->bufferListLen*sizeof(PULONG_PTR));
		status = STATUS_SUCCESS;
	} else {
		//内存不足
		status = STATUS_INSUFFICIENT_RESOURCES;
	}
	return  status;
}

NTSTATUS LCXLBitmapSet(
	IN OUT PLCXL_BITMAP bitmap,	 // 位图句柄指针
	IN ULONGLONG FirstBitmapIndex,// 从位图序号开始，以位为单位
	IN ULONGLONG BitmapNum,		//要写入BitmapNum位的数据
	IN BOOLEAN IsSetTo1			//是否设置为1
	)
{
	ULONGLONG BitmapUlongPtrIndex;//位图数据的序号，以ULONG_PTR为单位
	BYTE BitmapBitIndex;//在字节中的序号

	ULONG BitmapBufferIndex;//获取位图数据所在的列表序号
	ULONG BitmapBufferUlongPtrIndex;//获取位图数据所在的列表序号中的哪个ULONG_PTR中
	//ULONGLONG i;
	//如果位图序号超过了位图大小，返回错误
	if (FirstBitmapIndex+BitmapNum>bitmap->BitmapSize) {
		//这个错误代码值得商量。。。
		return STATUS_FWP_OUT_OF_BOUNDS;
	}
	for (; BitmapNum>0; FirstBitmapIndex++, BitmapNum--) {
		BitmapUlongPtrIndex = FirstBitmapIndex/(sizeof(ULONG_PTR)<<3);//获取此位图序号在哪个字节上
		BitmapBitIndex = FirstBitmapIndex%(sizeof(ULONG_PTR)<<3);//获取此位图序号在字节上的位置
		BitmapBufferIndex = (ULONG)(BitmapUlongPtrIndex/bitmap->regionUlongPtr);//获取此位图序号在哪个列表序号中
		BitmapBufferUlongPtrIndex = BitmapUlongPtrIndex%bitmap->regionUlongPtr;//获取此位图序号在列表中的哪个ULONG_PTR位置上
		//如果这个区域的位图还没有
		if (bitmap->bufferList[BitmapBufferIndex] == NULL) {
			//如果置0，则不需要额外的操作了
			//默认整个位图都为0
			if (!IsSetTo1) {
				continue;
			} else {
				//申请内存，准备存数据
				bitmap->bufferList[BitmapBufferIndex] = (PULONG_PTR)ExAllocatePoolWithTag(PagedPool, bitmap->regionUlongPtr*sizeof(ULONG_PTR), INFO_BITMAP_TAG);
				if (bitmap->bufferList[BitmapBufferIndex] != NULL) {
					//全部置零
					RtlZeroMemory(bitmap->bufferList[BitmapBufferIndex], bitmap->regionUlongPtr*sizeof(ULONG_PTR));
				} else {
					//内存不足
					return STATUS_INSUFFICIENT_RESOURCES;
				}
			}
		}

		//如果置为1
		if (IsSetTo1) {
			//设置位图数据，将对应的数据置为1
			bitmap->bufferList[BitmapBufferIndex][BitmapBufferUlongPtrIndex] |= (ULONG_PTR)1<<BitmapBitIndex;
		} else {
			//否则
			bitmap->bufferList[BitmapBufferIndex][BitmapBufferUlongPtrIndex] &= ~((ULONG_PTR)1<<BitmapBitIndex);
		}
	}
	
	return STATUS_SUCCESS;
}

//获取位图数据
BOOLEAN LCXLBitmapGet(
	IN PLCXL_BITMAP bitmap,	 // 位图句柄指针
	IN ULONGLONG BitmapIndex // 位图序号，以位为单位
	)
{
	ULONGLONG BitmapUlongPtrIndex;//位图数据的序号，以ULONG_PTR为单位
	BYTE BitmapBitIndex;//在字节中的序号

	ULONG BitmapBufferIndex;//获取位图数据所在的列表序号
	ULONG BitmapBufferUlongPtrIndex;//获取位图数据所在的列表序号中的哪个ULONG_PTR中

	//如果位图序号超过了位图大小，返回错误
	if (BitmapIndex>=bitmap->BitmapSize) {
		KdPrint(("SYS:!!!LCXLBitmapGet:BitmapIndex(%I64d)>=bitmap->BitmapSize(%I64d)\n", BitmapIndex, bitmap->BitmapSize));
		return FALSE;
	}

	BitmapUlongPtrIndex = BitmapIndex/(sizeof(ULONG_PTR)<<3);//获取此位图序号在哪个字节上
	BitmapBitIndex = BitmapIndex%(sizeof(ULONG_PTR)<<3);//获取此位图序号在字节上的位置
	BitmapBufferIndex = (ULONG)(BitmapUlongPtrIndex/bitmap->regionUlongPtr);//获取此位图序号在哪个列表序号中
	BitmapBufferUlongPtrIndex = BitmapUlongPtrIndex%bitmap->regionUlongPtr;//获取此位图序号在列表中的哪个ULONG_PTR位置上
	//如果这个区域的位图还没有
	if (bitmap->bufferList[BitmapBufferIndex] == NULL) {
		//返回0
		return FALSE;
	}
	return ((bitmap->bufferList[BitmapBufferIndex][BitmapBufferUlongPtrIndex]&((ULONG_PTR)1<<BitmapBitIndex))!=0);
}

//释放位图结构
VOID LCXLBitmapFina(
	IN OUT PLCXL_BITMAP bitmap	 // 位图句柄指针
	)
{
	PAGED_CODE();

	ASSERT(bitmap!=NULL);
	if (bitmap->bufferList != NULL) {
		ULONG i;
		for (i = 0; i < bitmap->bufferListLen; i++) {
			if (bitmap->bufferList[i] != NULL) {
				ExFreePoolWithTag(bitmap->bufferList[i], INFO_BITMAP_TAG);
			}
		}
		ExFreePoolWithTag(bitmap->bufferList, INFO_BITMAP_TAG);
	}
	//将整个结构置零
	RtlZeroMemory(bitmap, sizeof(LCXL_BITMAP));
}

//寻找空闲的位(即为0的位)的序号
//如果返回的序号为BitmapSize，则表明位图已用完
ULONGLONG LCXLBitmapFindFreeBit(
	IN PLCXL_BITMAP LcxlBitmap,// 位图数据指针
	IN ULONGLONG StartOffset //从StartOffset处开始找，实际中它会以StartOffset/sizeof(ULONG_PTR)*8/(sizeof(ULONG_PTR)*8)，也就是按ULONG_PTR对齐，提高速度
	)
{
	ULONGLONG BitmapUlongPtrIndex;//位图数据的序号，以ULONG_PTR为单位
	//BYTE BitmapBitIndex;//在字节中的序号

	ULONG BitmapBufferIndex;//获取位图数据所在的列表序号
	ULONG BitmapBufferUlongPtrIndex;//获取位图数据所在的列表序号中的哪个ULONG_PTR中
	ULONGLONG i;
	BYTE j;
	ULONG_PTR tmp;

	ASSERT(LcxlBitmap);
	//使用ULONG_PTR，提高效率
	BitmapUlongPtrIndex = StartOffset/(sizeof(ULONG_PTR)<<3);//获取此位图序号在哪个字节上
	//下面都是
	for (i = BitmapUlongPtrIndex; i < LcxlBitmap->BitmapSize/(sizeof(ULONG_PTR)<<3); i++) {
		BitmapBufferIndex = (ULONG)(i/LcxlBitmap->regionUlongPtr);//获取此位图序号在哪个列表序号中
		
		if (LcxlBitmap->bufferList[BitmapBufferIndex] != NULL) {
			BitmapBufferUlongPtrIndex = i%LcxlBitmap->regionUlongPtr;//获取此位图序号在列表中的哪个ULONG_PTR位置上
			tmp = LcxlBitmap->bufferList[BitmapBufferIndex][BitmapBufferUlongPtrIndex];
			if (tmp!=~((ULONG_PTR)0)) {
				//如果有0值
				for (j = 0; j < sizeof(ULONG_PTR)<<3; j++) {
					if ((tmp&((ULONG_PTR)1<<j))==0) {
						return (i*sizeof(ULONG_PTR)<<3)+j;
					}
				}
				//不应该找不到的
				ASSERT(0);
			}
		} else {
			return i*sizeof(ULONG_PTR)<<3;
		}
	}
	//找到末尾了，对最后ULONG_PTR进行处理
	BitmapBufferIndex = (ULONG)(i/LcxlBitmap->regionUlongPtr);//获取此位图序号在哪个列表序号中
	if (LcxlBitmap->bufferList[BitmapBufferIndex] != NULL) {
		BitmapBufferUlongPtrIndex = i%LcxlBitmap->regionUlongPtr;//获取此位图序号在列表中的哪个ULONG_PTR位置上
		tmp = LcxlBitmap->bufferList[BitmapBufferIndex][BitmapBufferUlongPtrIndex];
		for (j = 0; j < LcxlBitmap->BitmapSize%(sizeof(ULONG_PTR)<<3); j++) {
			if ((tmp&((ULONG_PTR)1<<j))==0) {
				return (i*sizeof(ULONG_PTR)<<3)+j;
			}
		}
	} else {
		return i*sizeof(ULONG_PTR)<<3;
	}
	//位图中空闲位不存在
	return LcxlBitmap->BitmapSize;
}

//通过Bitmap创建LCXLBitmap位图（LCXL位图的大小为BitmapSize+BitmapOffset）
NTSTATUS LCXLBitmapCreateFromBitmap(
	IN PBYTE bitmap,		   // 位图数据指针
	IN ULONGLONG BitmapSize,   // 位图大小，以位为单位
	IN ULONGLONG BitmapOffset, // bitmap位图在LCXLBitmap位图的偏移量
	IN ULONG regionBytes,	   // 位图粒度，分成N块，一块占多少byte
	OUT PLCXL_BITMAP LcxlBitmap// 新的LCXL位图
	)
{
	NTSTATUS status;
	//在位图大小的基础上再增加8位
	static INT reInt = 8;
	status = LCXLBitmapInit(LcxlBitmap, BitmapSize+BitmapOffset+reInt, regionBytes);
	if (NT_SUCCESS(status)) {
		ULONGLONG i;
		ULONGLONG j;

		for (j = 0, i = BitmapOffset; i<BitmapSize; i++, j++) {
			ULONG ByteIndex;
			ULONG BitIndex;

			ByteIndex = (ULONG)(j/8);
			BitIndex = j%8;
			status = LCXLBitmapSet(LcxlBitmap, i, 1, bitmap[ByteIndex]&(1<<BitIndex));
		}
	}
	return status;
}