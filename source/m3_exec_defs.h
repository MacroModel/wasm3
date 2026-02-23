//
//  m3_exec_defs.h
//
//  Created by Steven Massey on 5/1/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_exec_defs_h
#define m3_exec_defs_h

#include "m3_core.h"

d_m3BeginExternC

# define m3MemData(mem)                 (u8*)(((M3MemoryHeader*)(mem))+1)
# define m3MemRuntime(mem)              (((M3MemoryHeader*)(mem))->runtime)
# define m3MemInfo(mem)                 (&(((M3MemoryHeader*)(mem))->runtime->memory))

# if d_m3EnableLocalRegCaching
#   if !defined(__aarch64__)
#     error "d_m3EnableLocalRegCaching is currently only supported on AArch64"
#   endif
    // AAPCS64 has 8 integer argument registers (x0-x7). wasm3 uses 4 fixed args (_pc/_sp/_mem/_r0),
    // leaving 4 to cache hot locals (_r1.._r4).
#   define d_m3BaseOpSig                  pc_t _pc, m3stack_t _sp, M3MemoryHeader * _mem, m3reg_t _r0, m3reg_t _r1, m3reg_t _r2, m3reg_t _r3, m3reg_t _r4
#   define d_m3BaseOpArgs                 _sp, _mem, _r0, _r1, _r2, _r3, _r4
#   define d_m3BaseOpAllArgs              _pc, _sp, _mem, _r0, _r1, _r2, _r3, _r4
#   define d_m3BaseOpDefaultArgs          0, 0, 0, 0, 0
# else
#   define d_m3BaseOpSig                  pc_t _pc, m3stack_t _sp, M3MemoryHeader * _mem, m3reg_t _r0
#   define d_m3BaseOpArgs                 _sp, _mem, _r0
#   define d_m3BaseOpAllArgs              _pc, _sp, _mem, _r0
#   define d_m3BaseOpDefaultArgs          0
# endif
# define d_m3BaseClearRegisters         _r0 = 0;
# define d_m3BaseCstr                   ""

# define d_m3ExpOpSig(...)              d_m3BaseOpSig, __VA_ARGS__
# define d_m3ExpOpArgs(...)             d_m3BaseOpArgs, __VA_ARGS__
# define d_m3ExpOpAllArgs(...)          d_m3BaseOpAllArgs, __VA_ARGS__
# define d_m3ExpOpDefaultArgs(...)      d_m3BaseOpDefaultArgs, __VA_ARGS__
# define d_m3ExpClearRegisters(...)     d_m3BaseClearRegisters; __VA_ARGS__

# if d_m3HasFloat
#   if d_m3EnableLocalRegCaching
        // AAPCS64 has 8 FP/SIMD argument registers (v0-v7). wasm3 uses 1 fixed arg (_fp0),
        // leaving 7 to cache hot locals (_fp1.._fp7).
#       define d_m3OpSig                d_m3ExpOpSig            (f64 _fp0, f64 _fp1, f64 _fp2, f64 _fp3, f64 _fp4, f64 _fp5, f64 _fp6, f64 _fp7)
#       define d_m3OpArgs               d_m3ExpOpArgs           (_fp0, _fp1, _fp2, _fp3, _fp4, _fp5, _fp6, _fp7)
#       define d_m3OpAllArgs            d_m3ExpOpAllArgs        (_fp0, _fp1, _fp2, _fp3, _fp4, _fp5, _fp6, _fp7)
#       define d_m3OpDefaultArgs        d_m3ExpOpDefaultArgs    (0., 0., 0., 0., 0., 0., 0., 0.)
#       define d_m3ClearRegisters       d_m3ExpClearRegisters   (_fp0 = 0.;)
#   else
#       define d_m3OpSig                d_m3ExpOpSig            (f64 _fp0)
#       define d_m3OpArgs               d_m3ExpOpArgs           (_fp0)
#       define d_m3OpAllArgs            d_m3ExpOpAllArgs        (_fp0)
#       define d_m3OpDefaultArgs        d_m3ExpOpDefaultArgs    (0.)
#       define d_m3ClearRegisters       d_m3ExpClearRegisters   (_fp0 = 0.;)
#   endif
# else
#   define d_m3OpSig                d_m3BaseOpSig
#   define d_m3OpArgs               d_m3BaseOpArgs
#   define d_m3OpAllArgs            d_m3BaseOpAllArgs
#   define d_m3OpDefaultArgs        d_m3BaseOpDefaultArgs
#   define d_m3ClearRegisters       d_m3BaseClearRegisters
# endif

# if d_m3EnableLocalRegCaching
    // Slot offset encoding for cached locals.
    // Encoded offsets are negative i32 values in the code stream.
    // [31] tag (1)
    // [30] kind (0=int, 1=fp)
    // [29..26] reg index
    // [25..0] slot offset (in m3slot_t units)
    #define d_m3LocalOffsetTagMask       0x80000000u
    #define d_m3LocalOffsetKindMask      0x40000000u
    #define d_m3LocalOffsetRegShift      26u
    #define d_m3LocalOffsetRegMask       0x3Cu
    #define d_m3LocalOffsetSlotMask      0x03FFFFFFu

    static inline u32  m3EncodeLocalOffset  (u32 i_kind, u32 i_regIndex, u32 i_slotOffset)
    {
        return d_m3LocalOffsetTagMask | ((i_kind & 1u) << 30u) | ((i_regIndex & 0xFu) << d_m3LocalOffsetRegShift) | (i_slotOffset & d_m3LocalOffsetSlotMask);
    }

    static inline bool  m3IsEncodedLocalOffset  (i32 i_offset)
    {
        return i_offset < 0;
    }

    static inline u32  m3DecodeLocalOffsetKind  (u32 i_encoded)
    {
        return (i_encoded >> 30u) & 1u;
    }

    static inline u32  m3DecodeLocalOffsetRegIndex  (u32 i_encoded)
    {
        return (i_encoded >> d_m3LocalOffsetRegShift) & 0xFu;
    }

    static inline u32  m3DecodeLocalOffsetSlot  (u32 i_encoded)
    {
        return i_encoded & d_m3LocalOffsetSlotMask;
    }
# endif


#define d_m3RetSig                  static inline m3ret_t vectorcall
# if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    typedef m3ret_t (vectorcall * IM3Operation) (d_m3OpSig, cstr_t i_operationName);
#    define d_m3Op(NAME)                M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig, cstr_t i_operationName)

#    define nextOpImpl()            ((IM3Operation)(* _pc))(_pc + 1, d_m3OpArgs, __FUNCTION__)
#    define jumpOpImpl(PC)          ((IM3Operation)(*  PC))( PC + 1, d_m3OpArgs, __FUNCTION__)
# else
    typedef m3ret_t (vectorcall * IM3Operation) (d_m3OpSig);
#    define d_m3Op(NAME)                M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig)

#    define nextOpImpl()            ((IM3Operation)(* _pc))(_pc + 1, d_m3OpArgs)
#    define jumpOpImpl(PC)          ((IM3Operation)(*  PC))( PC + 1, d_m3OpArgs)
# endif

#define nextOpDirect()              M3_MUSTTAIL return nextOpImpl()
#define jumpOpDirect(PC)            M3_MUSTTAIL return jumpOpImpl((pc_t)(PC))

# if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig  RunCode  (d_m3OpSig, cstr_t i_operationName)
# else
d_m3RetSig  RunCode  (d_m3OpSig)
# endif
{
    nextOpDirect();
}

d_m3EndExternC

#endif // m3_exec_defs_h
