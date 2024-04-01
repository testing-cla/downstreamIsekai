#ifndef ISEKAI_COMMON_CONSTANTS_H_
#define ISEKAI_COMMON_CONSTANTS_H_

namespace isekai {

// Bitmap sizes for pkt/tx/rx request/data window.
// For bitmaps of ack packet and rx window, it can have different sizes for
// request and window. We uniformly use one size so that other functions do not
// need to be templated.
// Changing bitmap sizes might affect class declaration in falcon_bitmap.cc.
// In falcon_bitmap.cc FalconBitmap template class declaration, each bitmap size
// needs to be declared but only once. Currently the unique bitmap sizes include
// 128, 256 and 1024. As an example, if setting kGen2TxBitmapWidth = 512, it is
// necessary to add a class declaration with kGen2TxBitmapWidth.
constexpr int kAckPacketBitmapWidth = 128;
constexpr int kAckPacketRequestWindowSize = 64;
constexpr int kAckPacketDataWindowSize = 128;
constexpr int kRxBitmapWidth = kAckPacketBitmapWidth;
constexpr int kRxRequestWindowSize = 64;
constexpr int kRxDataWindowSize = 128;
//
constexpr int kTxBitmapWidth = 1024;
// Bitmap sizes for Falcon Gen2.
constexpr int kGen2RxBitmapWidth = 256;
constexpr int kGen2TxBitmapWidth = 1024;

}  // namespace isekai

#endif  // ISEKAI_COMMON_CONSTANTS_H_
