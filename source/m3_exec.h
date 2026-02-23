//
//  m3_exec.h
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.


#ifndef m3_exec_h
#define m3_exec_h

// TODO: all these functions could move over to the .c at some point. normally, I'd say screw it,
// but it might prove useful to be able to compile m3_exec alone w/ optimizations while the remaining
// code is at debug O0


// About the naming convention of these operations/macros (_rs, _sr_, _ss, _srs, etc.)
//------------------------------------------------------------------------------------------------------
//   - 'r' means register and 's' means slot
//   - the first letter is the top of the stack
//
//  so, for example, _rs means the first operand (the first thing pushed to the stack) is in a slot
//  and the second operand (the top of the stack) is in a register
//------------------------------------------------------------------------------------------------------

#ifndef M3_COMPILE_OPCODES
#  error "Opcodes should only be included in one compilation unit"
#endif

#include "m3_math_utils.h"
#include "m3_compile.h"
#include "m3_env.h"
#include "m3_info.h"
#include "m3_exec_defs.h"

#include <limits.h>
#if d_m3EnableLocalRegCaching && d_m3EnableLocalRegCachingValidate
#include <stdio.h>
#endif

d_m3BeginExternC

# define rewrite_op(OP)             * ((void **) (_pc-1)) = (void*)(OP)

# define immediate(TYPE)            * ((TYPE *) _pc++)
# define skip_immediate(TYPE)       (_pc++)

#if d_m3EnableLocalRegCaching
    #if !(defined(__clang__) || defined(__GNUC__))
        #error "d_m3EnableLocalRegCaching requires a compiler that supports statement expressions"
    #endif

    static inline u32  m3DecodeSlotOffset  (i32 i_offset)
    {
        if (M3_LIKELY(not m3IsEncodedLocalOffset (i_offset)))
            return (u32) i_offset;
        return m3DecodeLocalOffsetSlot ((u32) i_offset);
    }

    static inline u64  m3BitcastF64ToU64  (f64 i_value)
    {
        union { f64 f; u64 u; } u;
        u.f = i_value;
        return u.u;
    }

    static inline f64  m3BitcastU64ToF64  (u64 i_value)
    {
        union { u64 u; f64 f; } u;
        u.u = i_value;
        return u.f;
    }

    #define M3_GET_LOCAL_INT_REG(REG)   \
        ((REG) == 0 ? _r1 : (REG) == 1 ? _r2 : (REG) == 2 ? _r3 : (REG) == 3 ? _r4 : 0)

    #if d_m3HasFloat
        #define M3_GET_LOCAL_FP_REG(REG)    \
            ((REG) == 0 ? _fp1 : (REG) == 1 ? _fp2 : (REG) == 2 ? _fp3 : (REG) == 3 ? _fp4 : (REG) == 4 ? _fp5 : (REG) == 5 ? _fp6 : (REG) == 6 ? _fp7 : 0.)
    #else
        #define M3_GET_LOCAL_FP_REG(REG)    (0.)
    #endif

    #define M3_SET_LOCAL_INT_REG(REG, VALUE)                 \
        do {                                                 \
            switch (REG) {                                   \
                case 0: _r1 = (m3reg_t) (VALUE); break;      \
                case 1: _r2 = (m3reg_t) (VALUE); break;      \
                case 2: _r3 = (m3reg_t) (VALUE); break;      \
                case 3: _r4 = (m3reg_t) (VALUE); break;      \
                default: break;                              \
            }                                                \
        } while (0)

    #if d_m3HasFloat
        #define M3_SET_LOCAL_FP_REG(REG, VALUE)                  \
            do {                                                 \
                switch (REG) {                                   \
                    case 0: _fp1 = (f64) (VALUE); break;         \
                    case 1: _fp2 = (f64) (VALUE); break;         \
                    case 2: _fp3 = (f64) (VALUE); break;         \
                    case 3: _fp4 = (f64) (VALUE); break;         \
                    case 4: _fp5 = (f64) (VALUE); break;         \
                    case 5: _fp6 = (f64) (VALUE); break;         \
                    case 6: _fp7 = (f64) (VALUE); break;         \
                    default: break;                              \
                }                                                \
            } while (0)
    #else
        #define M3_SET_LOCAL_FP_REG(REG, VALUE)                  \
            do { (void) (REG); (void) (VALUE); } while (0)
    #endif

    #define M3_SLOT_LOAD(TYPE, OFFSET_EXPR)                                             \
        ({                                                                              \
            const i32 _m3_off = (OFFSET_EXPR);                                          \
            TYPE _m3_val;                                                               \
            if (M3_LIKELY(_m3_off >= 0))                                                \
                _m3_val = * (TYPE *) (_sp + (u32) _m3_off);                             \
            else                                                                        \
            {                                                                           \
                const u32 _m3_enc = (u32) _m3_off;                                      \
                const u32 _m3_kind = m3DecodeLocalOffsetKind (_m3_enc);                 \
                const u32 _m3_reg = m3DecodeLocalOffsetRegIndex (_m3_enc);              \
                if (_m3_kind)                                                          \
                {                                                                       \
                    const f64 _m3_fp = (f64) M3_GET_LOCAL_FP_REG (_m3_reg);             \
                    if (__builtin_types_compatible_p (TYPE, u64) || __builtin_types_compatible_p (TYPE, i64)) \
                        _m3_val = (TYPE) m3BitcastF64ToU64 (_m3_fp);                    \
                    else                                                                \
                        _m3_val = (TYPE) _m3_fp;                                       \
                }                                                                       \
                else                                                                    \
                    _m3_val = (TYPE) M3_GET_LOCAL_INT_REG (_m3_reg);                   \
                if (d_m3EnableLocalRegCachingValidate)                                  \
                {                                                                       \
                    const u32 _m3_slot = m3DecodeLocalOffsetSlot (_m3_enc);             \
                    const TYPE _m3_mem_val = * (TYPE *) (_sp + _m3_slot);               \
                    if (M3_UNLIKELY(memcmp (& _m3_mem_val, & _m3_val, sizeof (TYPE)) != 0)) \
                    {                                                                   \
                        fprintf (stderr, "m3 local-regcache mismatch: kind=%u slot=%u reg=%u off=%d\\n", _m3_kind, _m3_slot, _m3_reg, _m3_off); \
                        m3_Abort ("local-regcache mismatch");                           \
                    }                                                                   \
                }                                                                       \
            }                                                                           \
            _m3_val;                                                                    \
        })

    #define M3_SLOT_STORE(TYPE, OFFSET_EXPR, VALUE_EXPR)                                \
        do {                                                                            \
            const i32 _m3_off = (OFFSET_EXPR);                                          \
            const TYPE _m3_val = (TYPE) (VALUE_EXPR);                                   \
            if (M3_LIKELY(_m3_off >= 0))                                                \
            {                                                                           \
                * (TYPE *) (_sp + (u32) _m3_off) = _m3_val;                             \
            }                                                                           \
            else                                                                        \
            {                                                                           \
                const u32 _m3_enc = (u32) _m3_off;                                      \
                const u32 _m3_kind = m3DecodeLocalOffsetKind (_m3_enc);                 \
                const u32 _m3_reg = m3DecodeLocalOffsetRegIndex (_m3_enc);              \
                const u32 _m3_slot = m3DecodeLocalOffsetSlot (_m3_enc);                 \
                if (_m3_kind)                                                           \
                {                                                                       \
                    if (__builtin_types_compatible_p (TYPE, u64) || __builtin_types_compatible_p (TYPE, i64)) \
                        M3_SET_LOCAL_FP_REG (_m3_reg, m3BitcastU64ToF64 ((u64) _m3_val)); \
                    else                                                                \
                        M3_SET_LOCAL_FP_REG (_m3_reg, _m3_val);                         \
                }                                                                       \
                else                                                                    \
                    M3_SET_LOCAL_INT_REG (_m3_reg, _m3_val);                            \
                * (TYPE *) (_sp + _m3_slot) = _m3_val;                                  \
            }                                                                           \
        } while (0)

    #define slot(TYPE)                 M3_SLOT_LOAD (TYPE, immediate (i32))
    #define slot_ptr(TYPE)             (TYPE *) (_sp + m3DecodeSlotOffset (immediate (i32)))
    #define slot_store(TYPE, VALUE)    M3_SLOT_STORE (TYPE, immediate (i32), (VALUE))
#else
    #define M3_SLOT_LOAD(TYPE, OFFSET_EXPR)                 (* (TYPE *) (_sp + (OFFSET_EXPR)))
    #define M3_SLOT_STORE(TYPE, OFFSET_EXPR, VALUE_EXPR)    (* (TYPE *) (_sp + (OFFSET_EXPR)) = (TYPE) (VALUE_EXPR))

    #define slot(TYPE)                 * (TYPE *) (_sp + immediate (i32))
    #define slot_ptr(TYPE)             (TYPE *) (_sp + immediate (i32))
    #define slot_store(TYPE, VALUE)    (slot (TYPE) = (TYPE) (VALUE))
#endif

#if d_m3EnableLocalRegCaching
static inline m3reg_t  m3LoadLocalInt  (m3stack_t i_sp, u16 i_slot, u8 i_type)
{
    if (i_type == c_m3Type_i32)
        return (m3reg_t) * (i32 *) (i_sp + i_slot);
    else
        return (m3reg_t) * (i64 *) (i_sp + i_slot);
}

#if d_m3HasFloat
static inline f64  m3LoadLocalFp  (m3stack_t i_sp, u16 i_slot, u8 i_type)
{
    if (i_type == c_m3Type_f32)
        return (f64) * (f32 *) (i_sp + i_slot);
    else
        return * (f64 *) (i_sp + i_slot);
}
#endif

#define M3_CLEAR_LOCAL_INT_REGS()                 \
    do {                                          \
        _r1 = 0; _r2 = 0; _r3 = 0; _r4 = 0;        \
    } while (0)

#if d_m3HasFloat
    #define M3_CLEAR_LOCAL_FP_REGS()                  \
        do {                                           \
            _fp1 = 0.; _fp2 = 0.; _fp3 = 0.; _fp4 = 0.; \
            _fp5 = 0.; _fp6 = 0.; _fp7 = 0.;           \
        } while (0)
#else
    #define M3_CLEAR_LOCAL_FP_REGS()                  \
        do {} while (0)
#endif

#define M3_CLEAR_LOCAL_REGS()                     \
    do {                                          \
        M3_CLEAR_LOCAL_INT_REGS ();               \
        M3_CLEAR_LOCAL_FP_REGS ();                \
    } while (0)

#if d_m3HasFloat
    #define M3_RELOAD_LOCAL_REGS(FUNCTION)                                        \
        do {                                                                      \
            M3_CLEAR_LOCAL_REGS ();                                               \
                                                                                  \
            const u8 _m3_ni = (FUNCTION)->numLocalIntRegs;                        \
            if (_m3_ni > 0) _r1 = m3LoadLocalInt (_sp, (FUNCTION)->localIntRegSlots[0], (FUNCTION)->localIntRegTypes[0]); \
            if (_m3_ni > 1) _r2 = m3LoadLocalInt (_sp, (FUNCTION)->localIntRegSlots[1], (FUNCTION)->localIntRegTypes[1]); \
            if (_m3_ni > 2) _r3 = m3LoadLocalInt (_sp, (FUNCTION)->localIntRegSlots[2], (FUNCTION)->localIntRegTypes[2]); \
            if (_m3_ni > 3) _r4 = m3LoadLocalInt (_sp, (FUNCTION)->localIntRegSlots[3], (FUNCTION)->localIntRegTypes[3]); \
                                                                                  \
            const u8 _m3_nf = (FUNCTION)->numLocalFpRegs;                         \
            if (_m3_nf > 0) _fp1 = m3LoadLocalFp (_sp, (FUNCTION)->localFpRegSlots[0], (FUNCTION)->localFpRegTypes[0]);    \
            if (_m3_nf > 1) _fp2 = m3LoadLocalFp (_sp, (FUNCTION)->localFpRegSlots[1], (FUNCTION)->localFpRegTypes[1]);    \
            if (_m3_nf > 2) _fp3 = m3LoadLocalFp (_sp, (FUNCTION)->localFpRegSlots[2], (FUNCTION)->localFpRegTypes[2]);    \
            if (_m3_nf > 3) _fp4 = m3LoadLocalFp (_sp, (FUNCTION)->localFpRegSlots[3], (FUNCTION)->localFpRegTypes[3]);    \
            if (_m3_nf > 4) _fp5 = m3LoadLocalFp (_sp, (FUNCTION)->localFpRegSlots[4], (FUNCTION)->localFpRegTypes[4]);    \
            if (_m3_nf > 5) _fp6 = m3LoadLocalFp (_sp, (FUNCTION)->localFpRegSlots[5], (FUNCTION)->localFpRegTypes[5]);    \
            if (_m3_nf > 6) _fp7 = m3LoadLocalFp (_sp, (FUNCTION)->localFpRegSlots[6], (FUNCTION)->localFpRegTypes[6]);    \
        } while (0)
#else
    #define M3_RELOAD_LOCAL_REGS(FUNCTION)                                        \
        do {                                                                      \
            M3_CLEAR_LOCAL_REGS ();                                               \
                                                                                  \
            const u8 _m3_ni = (FUNCTION)->numLocalIntRegs;                        \
            if (_m3_ni > 0) _r1 = m3LoadLocalInt (_sp, (FUNCTION)->localIntRegSlots[0], (FUNCTION)->localIntRegTypes[0]); \
            if (_m3_ni > 1) _r2 = m3LoadLocalInt (_sp, (FUNCTION)->localIntRegSlots[1], (FUNCTION)->localIntRegTypes[1]); \
            if (_m3_ni > 2) _r3 = m3LoadLocalInt (_sp, (FUNCTION)->localIntRegSlots[2], (FUNCTION)->localIntRegTypes[2]); \
            if (_m3_ni > 3) _r4 = m3LoadLocalInt (_sp, (FUNCTION)->localIntRegSlots[3], (FUNCTION)->localIntRegTypes[3]); \
        } while (0)
#endif
#endif // d_m3EnableLocalRegCaching


# if d_m3EnableOpProfiling
                                    d_m3RetSig  profileOp   (d_m3OpSig, cstr_t i_operationName);
#   define nextOp()                 M3_MUSTTAIL return profileOp (d_m3OpAllArgs, __FUNCTION__)
# elif d_m3EnableOpTracing
                                    d_m3RetSig  debugOp     (d_m3OpSig, cstr_t i_operationName);
#   define nextOp()                 M3_MUSTTAIL return debugOp (d_m3OpAllArgs, __FUNCTION__)
# else
#   define nextOp()                 nextOpDirect()
# endif

#define jumpOp(PC)                  jumpOpDirect(PC)

#if d_m3RecordBacktraces
    #define pushBacktraceFrame()            (PushBacktraceFrame (_mem->runtime, _pc - 1))
    #define fillBacktraceFrame(FUNCTION)    (FillBacktraceFunctionInfo (_mem->runtime, function))

    #define newTrap(err)                    return (pushBacktraceFrame (), err)
    #define forwardTrap(err)                return err
#else
    #define pushBacktraceFrame()            do {} while (0)
    #define fillBacktraceFrame(FUNCTION)    do {} while (0)

    #define newTrap(err)                    return err
    #define forwardTrap(err)                return err
#endif


#if d_m3EnableStrace == 1
    // Flat trace
    #define d_m3TracePrepare
    #define d_m3TracePrint(fmt, ...)            fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#elif d_m3EnableStrace >= 2
    // Structured trace
    #define d_m3TracePrepare                    const IM3Runtime trace_rt = m3MemRuntime(_mem);
    #define d_m3TracePrint(fmt, ...)            fprintf(stderr, "%*s" fmt "\n", (trace_rt->callDepth)*2, "", ##__VA_ARGS__)
#else
    #define d_m3TracePrepare
    #define d_m3TracePrint(fmt, ...)
#endif

#if d_m3EnableStrace >= 3
    #define d_m3TraceLoad(TYPE,offset,val)      d_m3TracePrint("load." #TYPE "  0x%x = %" PRI##TYPE, offset, val)
    #define d_m3TraceStore(TYPE,offset,val)     d_m3TracePrint("store." #TYPE " 0x%x , %" PRI##TYPE, offset, val)
#else
    #define d_m3TraceLoad(TYPE,offset,val)
    #define d_m3TraceStore(TYPE,offset,val)
#endif

#ifdef DEBUG
  #define d_outOfBounds newTrap (ErrorRuntime (m3Err_trapOutOfBoundsMemoryAccess,   \
                        _mem->runtime, "op: %s; memory size: %zu; access offset: %zu", \
                        __FUNCTION__, _mem->length, operand))

#   define d_outOfBoundsMemOp(OFFSET, SIZE) newTrap (ErrorRuntime (m3Err_trapOutOfBoundsMemoryAccess,   \
                      _mem->runtime, "op: %s; memory size: %zu; access offset: %zu; size: %u",     \
                      __FUNCTION__, _mem->length, OFFSET, SIZE))
#else
  #define d_outOfBounds newTrap (m3Err_trapOutOfBoundsMemoryAccess)

#   define d_outOfBoundsMemOp(OFFSET, SIZE) newTrap (m3Err_trapOutOfBoundsMemoryAccess)

#endif

# if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig  Call  (d_m3OpSig, cstr_t i_operationName)
# else
d_m3RetSig  Call  (d_m3OpSig)
# endif
{
    m3ret_t possible_trap = m3_Yield ();
    if (M3_UNLIKELY(possible_trap)) return possible_trap;

    nextOpDirect();
}

// TODO: OK, this needs some explanation here ;0

#define d_m3CommutativeOpMacro(RES, REG, TYPE, NAME, OP, ...) \
d_m3Op(TYPE##_##NAME##_rs)                              \
{                                                       \
    TYPE operand = slot (TYPE);                         \
    OP((RES), operand, ((TYPE) REG), ##__VA_ARGS__);    \
    nextOp ();                                          \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss)                              \
{                                                       \
    TYPE operand2 = slot (TYPE);                        \
    TYPE operand1 = slot (TYPE);                        \
    OP((RES), operand1, operand2, ##__VA_ARGS__);       \
    nextOp ();                                          \
}

#define d_m3OpMacro(RES, REG, TYPE, NAME, OP, ...)      \
d_m3Op(TYPE##_##NAME##_sr)                              \
{                                                       \
    TYPE operand = slot (TYPE);                         \
    OP((RES), ((TYPE) REG), operand, ##__VA_ARGS__);    \
    nextOp ();                                          \
}                                                       \
d_m3CommutativeOpMacro(RES, REG, TYPE,NAME, OP, ##__VA_ARGS__)

// Accept macros
#define d_m3CommutativeOpMacro_i(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#define d_m3OpMacro_i(TYPE, NAME, MACRO, ...)               d_m3OpMacro             ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#define d_m3CommutativeOpMacro_f(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#define d_m3OpMacro_f(TYPE, NAME, MACRO, ...)               d_m3OpMacro             (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)

#define M3_FUNC(RES, A, B, OP)  (RES) = OP((A), (B))        // Accept functions: res = OP(a,b)
#define M3_OPER(RES, A, B, OP)  (RES) = ((A) OP (B))        // Accept operators: res = a OP b

#define d_m3CommutativeOpFunc_i(TYPE, NAME, OP)     d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_FUNC, OP)
#define d_m3OpFunc_i(TYPE, NAME, OP)                d_m3OpMacro_i               (TYPE, NAME, M3_FUNC, OP)
#define d_m3CommutativeOpFunc_f(TYPE, NAME, OP)     d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_FUNC, OP)
#define d_m3OpFunc_f(TYPE, NAME, OP)                d_m3OpMacro_f               (TYPE, NAME, M3_FUNC, OP)

#define d_m3CommutativeOp_i(TYPE, NAME, OP)         d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_OPER, OP)
#define d_m3Op_i(TYPE, NAME, OP)                    d_m3OpMacro_i               (TYPE, NAME, M3_OPER, OP)
#define d_m3CommutativeOp_f(TYPE, NAME, OP)         d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER, OP)
#define d_m3Op_f(TYPE, NAME, OP)                    d_m3OpMacro_f               (TYPE, NAME, M3_OPER, OP)

// compare needs to be distinct for fp 'cause the result must be _r0
#define d_m3CompareOp_f(TYPE, NAME, OP)             d_m3OpMacro                 (_r0, _fp0, TYPE, NAME, M3_OPER, OP)
#define d_m3CommutativeCmpOp_f(TYPE, NAME, OP)      d_m3CommutativeOpMacro      (_r0, _fp0, TYPE, NAME, M3_OPER, OP)


//-----------------------

// signed
d_m3CommutativeOp_i (i32, Equal,            ==)     d_m3CommutativeOp_i (i64, Equal,            ==)
d_m3CommutativeOp_i (i32, NotEqual,         !=)     d_m3CommutativeOp_i (i64, NotEqual,         !=)

d_m3Op_i (i32, LessThan,                    < )     d_m3Op_i (i64, LessThan,                    < )
d_m3Op_i (i32, GreaterThan,                 > )     d_m3Op_i (i64, GreaterThan,                 > )
d_m3Op_i (i32, LessThanOrEqual,             <=)     d_m3Op_i (i64, LessThanOrEqual,             <=)
d_m3Op_i (i32, GreaterThanOrEqual,          >=)     d_m3Op_i (i64, GreaterThanOrEqual,          >=)

// unsigned
d_m3Op_i (u32, LessThan,                    < )     d_m3Op_i (u64, LessThan,                    < )
d_m3Op_i (u32, GreaterThan,                 > )     d_m3Op_i (u64, GreaterThan,                 > )
d_m3Op_i (u32, LessThanOrEqual,             <=)     d_m3Op_i (u64, LessThanOrEqual,             <=)
d_m3Op_i (u32, GreaterThanOrEqual,          >=)     d_m3Op_i (u64, GreaterThanOrEqual,          >=)

#if d_m3HasFloat
d_m3CommutativeCmpOp_f (f32, Equal,         ==)     d_m3CommutativeCmpOp_f (f64, Equal,         ==)
d_m3CommutativeCmpOp_f (f32, NotEqual,      !=)     d_m3CommutativeCmpOp_f (f64, NotEqual,      !=)
d_m3CompareOp_f (f32, LessThan,             < )     d_m3CompareOp_f (f64, LessThan,             < )
d_m3CompareOp_f (f32, GreaterThan,          > )     d_m3CompareOp_f (f64, GreaterThan,          > )
d_m3CompareOp_f (f32, LessThanOrEqual,      <=)     d_m3CompareOp_f (f64, LessThanOrEqual,      <=)
d_m3CompareOp_f (f32, GreaterThanOrEqual,   >=)     d_m3CompareOp_f (f64, GreaterThanOrEqual,   >=)
#endif

#define OP_ADD_32(A,B) (i32)((u32)(A) + (u32)(B))
#define OP_ADD_64(A,B) (i64)((u64)(A) + (u64)(B))
#define OP_SUB_32(A,B) (i32)((u32)(A) - (u32)(B))
#define OP_SUB_64(A,B) (i64)((u64)(A) - (u64)(B))
#define OP_MUL_32(A,B) (i32)((u32)(A) * (u32)(B))
#define OP_MUL_64(A,B) (i64)((u64)(A) * (u64)(B))

d_m3CommutativeOpFunc_i (i32, Add,      OP_ADD_32)  d_m3CommutativeOpFunc_i (i64, Add,      OP_ADD_64)
d_m3CommutativeOpFunc_i (i32, Multiply, OP_MUL_32)  d_m3CommutativeOpFunc_i (i64, Multiply, OP_MUL_64)

d_m3OpFunc_i (i32, Subtract,            OP_SUB_32)  d_m3OpFunc_i (i64, Subtract,            OP_SUB_64)

#define OP_SHL_32(X,N) ((X) << ((u32)(N) % 32))
#define OP_SHL_64(X,N) ((X) << ((u64)(N) % 64))
#define OP_SHR_32(X,N) ((X) >> ((u32)(N) % 32))
#define OP_SHR_64(X,N) ((X) >> ((u64)(N) % 64))

d_m3OpFunc_i (u32, ShiftLeft,       OP_SHL_32)      d_m3OpFunc_i (u64, ShiftLeft,       OP_SHL_64)
d_m3OpFunc_i (i32, ShiftRight,      OP_SHR_32)      d_m3OpFunc_i (i64, ShiftRight,      OP_SHR_64)
d_m3OpFunc_i (u32, ShiftRight,      OP_SHR_32)      d_m3OpFunc_i (u64, ShiftRight,      OP_SHR_64)

d_m3CommutativeOp_i (u32, And,              &)
d_m3CommutativeOp_i (u32, Or,               |)
d_m3CommutativeOp_i (u32, Xor,              ^)

d_m3CommutativeOp_i (u64, And,              &)
d_m3CommutativeOp_i (u64, Or,               |)
d_m3CommutativeOp_i (u64, Xor,              ^)

#if d_m3HasFloat
d_m3CommutativeOp_f (f32, Add,              +)      d_m3CommutativeOp_f (f64, Add,              +)
d_m3CommutativeOp_f (f32, Multiply,         *)      d_m3CommutativeOp_f (f64, Multiply,         *)
d_m3Op_f (f32, Subtract,                    -)      d_m3Op_f (f64, Subtract,                    -)
d_m3Op_f (f32, Divide,                      /)      d_m3Op_f (f64, Divide,                      /)
#endif

d_m3OpFunc_i(u32, Rotl, rotl32)
d_m3OpFunc_i(u32, Rotr, rotr32)
d_m3OpFunc_i(u64, Rotl, rotl64)
d_m3OpFunc_i(u64, Rotr, rotr64)

d_m3OpMacro_i(u32, Divide, OP_DIV_U);
d_m3OpMacro_i(i32, Divide, OP_DIV_S, INT32_MIN);
d_m3OpMacro_i(u64, Divide, OP_DIV_U);
d_m3OpMacro_i(i64, Divide, OP_DIV_S, INT64_MIN);

d_m3OpMacro_i(u32, Remainder, OP_REM_U);
d_m3OpMacro_i(i32, Remainder, OP_REM_S, INT32_MIN);
d_m3OpMacro_i(u64, Remainder, OP_REM_U);
d_m3OpMacro_i(i64, Remainder, OP_REM_S, INT64_MIN);

#if d_m3HasFloat
d_m3OpFunc_f(f32, Min, min_f32);
d_m3OpFunc_f(f32, Max, max_f32);
d_m3OpFunc_f(f64, Min, min_f64);
d_m3OpFunc_f(f64, Max, max_f64);

d_m3OpFunc_f(f32, CopySign, copysignf);
d_m3OpFunc_f(f64, CopySign, copysign);
#endif

// Unary operations
// Note: This macro follows the principle of d_m3OpMacro

#define d_m3UnaryMacro(RES, REG, TYPE, NAME, OP, ...)   \
d_m3Op(TYPE##_##NAME##_r)                           \
{                                                   \
    OP((RES), (TYPE) REG, ##__VA_ARGS__);           \
    nextOp ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_s)                           \
{                                                   \
    TYPE operand = slot (TYPE);                     \
    OP((RES), operand, ##__VA_ARGS__);              \
    nextOp ();                                      \
}

#define M3_UNARY(RES, X, OP) (RES) = OP(X)
#define d_m3UnaryOp_i(TYPE, NAME, OPERATION)        d_m3UnaryMacro( _r0,  _r0, TYPE, NAME, M3_UNARY, OPERATION)
#define d_m3UnaryOp_f(TYPE, NAME, OPERATION)        d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY, OPERATION)

#if d_m3HasFloat
d_m3UnaryOp_f (f32, Abs,        fabsf);         d_m3UnaryOp_f (f64, Abs,        fabs);
d_m3UnaryOp_f (f32, Ceil,       ceilf);         d_m3UnaryOp_f (f64, Ceil,       ceil);
d_m3UnaryOp_f (f32, Floor,      floorf);        d_m3UnaryOp_f (f64, Floor,      floor);
d_m3UnaryOp_f (f32, Trunc,      truncf);        d_m3UnaryOp_f (f64, Trunc,      trunc);
d_m3UnaryOp_f (f32, Sqrt,       sqrtf);         d_m3UnaryOp_f (f64, Sqrt,       sqrt);
d_m3UnaryOp_f (f32, Nearest,    rintf);         d_m3UnaryOp_f (f64, Nearest,    rint);
d_m3UnaryOp_f (f32, Negate,     -);             d_m3UnaryOp_f (f64, Negate,     -);
#endif

#define OP_EQZ(x) ((x) == 0)

d_m3UnaryOp_i (i32, EqualToZero, OP_EQZ)
d_m3UnaryOp_i (i64, EqualToZero, OP_EQZ)

// clz(0), ctz(0) results are undefined for rest platforms, fix it
#if (defined(__i386__) || defined(__x86_64__)) && !(defined(__AVX2__) || (defined(__ABM__) && defined(__BMI__)))
    #define OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
    #define OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
    // for 64-bit instructions branchless approach more preferable
    #define OP_CLZ_64(x) (__builtin_clzll((x) | (1LL <<  0)) + OP_EQZ(x))
    #define OP_CTZ_64(x) (__builtin_ctzll((x) | (1LL << 63)) + OP_EQZ(x))
#elif defined(__ppc__) || defined(__ppc64__)
// PowerPC is defined for __builtin_clz(0) and __builtin_ctz(0).
// See (https://github.com/aquynh/capstone/blob/master/MathExtras.h#L99)
    #define OP_CLZ_32(x) __builtin_clz(x)
    #define OP_CTZ_32(x) __builtin_ctz(x)
    #define OP_CLZ_64(x) __builtin_clzll(x)
    #define OP_CTZ_64(x) __builtin_ctzll(x)
#else
    #define OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
    #define OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
    #define OP_CLZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_clzll(x))
    #define OP_CTZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_ctzll(x))
#endif

d_m3UnaryOp_i (u32, Clz, OP_CLZ_32)
d_m3UnaryOp_i (u64, Clz, OP_CLZ_64)

d_m3UnaryOp_i (u32, Ctz, OP_CTZ_32)
d_m3UnaryOp_i (u64, Ctz, OP_CTZ_64)

d_m3UnaryOp_i (u32, Popcnt, __builtin_popcount)
d_m3UnaryOp_i (u64, Popcnt, __builtin_popcountll)

#define OP_WRAP_I64(X) ((X) & 0x00000000ffffffff)

d_m3Op(i32_Wrap_i64_r)
{
    _r0 = OP_WRAP_I64((i64) _r0);
    nextOp ();
}

d_m3Op(i32_Wrap_i64_s)
{
    i64 operand = slot (i64);
    _r0 = OP_WRAP_I64(operand);
    nextOp ();
}

// Integer sign extension operations
#define OP_EXTEND8_S_I32(X)  ((int32_t)(int8_t)(X))
#define OP_EXTEND16_S_I32(X) ((int32_t)(int16_t)(X))
#define OP_EXTEND8_S_I64(X)  ((int64_t)(int8_t)(X))
#define OP_EXTEND16_S_I64(X) ((int64_t)(int16_t)(X))
#define OP_EXTEND32_S_I64(X) ((int64_t)(int32_t)(X))

d_m3UnaryOp_i (i32, Extend8_s,  OP_EXTEND8_S_I32)
d_m3UnaryOp_i (i32, Extend16_s, OP_EXTEND16_S_I32)
d_m3UnaryOp_i (i64, Extend8_s,  OP_EXTEND8_S_I64)
d_m3UnaryOp_i (i64, Extend16_s, OP_EXTEND16_S_I64)
d_m3UnaryOp_i (i64, Extend32_s, OP_EXTEND32_S_I64)

#define d_m3TruncMacro(DEST, SRC, TYPE, NAME, FROM, OP, ...)   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_r)                \
{                                                   \
    OP((DEST), (FROM) SRC, ##__VA_ARGS__);          \
    nextOp ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_s)                \
{                                                   \
    FROM * stack = slot_ptr (FROM);                 \
    OP((DEST), (* stack), ##__VA_ARGS__);           \
    nextOp ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_r)                \
{                                                   \
    TYPE * dest = slot_ptr (TYPE);                  \
    OP((* dest), (FROM) SRC, ##__VA_ARGS__);        \
    nextOp ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_s)                \
{                                                   \
    FROM * stack = slot_ptr (FROM);                 \
    TYPE * dest = slot_ptr (TYPE);                  \
    OP((* dest), (* stack), ##__VA_ARGS__);         \
    nextOp ();                                      \
}

#if d_m3HasFloat
d_m3TruncMacro(_r0, _fp0, i32, Trunc, f32, OP_I32_TRUNC_F32)
d_m3TruncMacro(_r0, _fp0, u32, Trunc, f32, OP_U32_TRUNC_F32)
d_m3TruncMacro(_r0, _fp0, i32, Trunc, f64, OP_I32_TRUNC_F64)
d_m3TruncMacro(_r0, _fp0, u32, Trunc, f64, OP_U32_TRUNC_F64)

d_m3TruncMacro(_r0, _fp0, i64, Trunc, f32, OP_I64_TRUNC_F32)
d_m3TruncMacro(_r0, _fp0, u64, Trunc, f32, OP_U64_TRUNC_F32)
d_m3TruncMacro(_r0, _fp0, i64, Trunc, f64, OP_I64_TRUNC_F64)
d_m3TruncMacro(_r0, _fp0, u64, Trunc, f64, OP_U64_TRUNC_F64)

d_m3TruncMacro(_r0, _fp0, i32, TruncSat, f32, OP_I32_TRUNC_SAT_F32)
d_m3TruncMacro(_r0, _fp0, u32, TruncSat, f32, OP_U32_TRUNC_SAT_F32)
d_m3TruncMacro(_r0, _fp0, i32, TruncSat, f64, OP_I32_TRUNC_SAT_F64)
d_m3TruncMacro(_r0, _fp0, u32, TruncSat, f64, OP_U32_TRUNC_SAT_F64)

d_m3TruncMacro(_r0, _fp0, i64, TruncSat, f32, OP_I64_TRUNC_SAT_F32)
d_m3TruncMacro(_r0, _fp0, u64, TruncSat, f32, OP_U64_TRUNC_SAT_F32)
d_m3TruncMacro(_r0, _fp0, i64, TruncSat, f64, OP_I64_TRUNC_SAT_F64)
d_m3TruncMacro(_r0, _fp0, u64, TruncSat, f64, OP_U64_TRUNC_SAT_F64)
#endif

#define d_m3TypeModifyOp(REG_TO, REG_FROM, TO, NAME, FROM)  \
d_m3Op(TO##_##NAME##_##FROM##_r)                            \
{                                                           \
    REG_TO = (TO) ((FROM) REG_FROM);                        \
    nextOp ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = slot (FROM);                                \
    REG_TO = (TO) (from);                                   \
    nextOp ();                                              \
}

// Int to int
d_m3TypeModifyOp (_r0, _r0, i64, Extend, i32);
d_m3TypeModifyOp (_r0, _r0, i64, Extend, u32);

// Float to float
#if d_m3HasFloat
d_m3TypeModifyOp (_fp0, _fp0, f32, Demote, f64);
d_m3TypeModifyOp (_fp0, _fp0, f64, Promote, f32);
#endif

#define d_m3TypeConvertOp(REG_TO, REG_FROM, TO, NAME, FROM) \
d_m3Op(TO##_##NAME##_##FROM##_r_r)                          \
{                                                           \
    REG_TO = (TO) ((FROM) REG_FROM);                        \
    nextOp ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_s_r)                          \
{                                                           \
    slot_store (TO, (TO) ((FROM) REG_FROM));                \
    nextOp ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_r_s)                          \
{                                                           \
    FROM from = slot (FROM);                                \
    REG_TO = (TO) (from);                                   \
    nextOp ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_s_s)                          \
{                                                           \
    FROM from = slot (FROM);                                \
    slot_store (TO, (TO) (from));                           \
    nextOp ();                                              \
}

// Int to float
#if d_m3HasFloat
d_m3TypeConvertOp (_fp0, _r0, f64, Convert, i32);
d_m3TypeConvertOp (_fp0, _r0, f64, Convert, u32);
d_m3TypeConvertOp (_fp0, _r0, f64, Convert, i64);
d_m3TypeConvertOp (_fp0, _r0, f64, Convert, u64);

d_m3TypeConvertOp (_fp0, _r0, f32, Convert, i32);
d_m3TypeConvertOp (_fp0, _r0, f32, Convert, u32);
d_m3TypeConvertOp (_fp0, _r0, f32, Convert, i64);
d_m3TypeConvertOp (_fp0, _r0, f32, Convert, u64);
#endif

#define d_m3ReinterpretOp(REG, TO, SRC, FROM)               \
d_m3Op(TO##_Reinterpret_##FROM##_r_r)                       \
{                                                           \
    union { FROM c; TO t; } u;                              \
    u.c = (FROM) SRC;                                       \
    REG = u.t;                                              \
    nextOp ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_Reinterpret_##FROM##_r_s)                       \
{                                                           \
    union { FROM c; TO t; } u;                              \
    u.c = slot (FROM);                                      \
    REG = u.t;                                              \
    nextOp ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_Reinterpret_##FROM##_s_r)                       \
{                                                           \
    union { FROM c; TO t; } u;                              \
    u.c = (FROM) SRC;                                       \
    slot_store (TO, u.t);                                   \
    nextOp ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_Reinterpret_##FROM##_s_s)                       \
{                                                           \
    union { FROM c; TO t; } u;                              \
    u.c = slot (FROM);                                      \
    slot_store (TO, u.t);                                   \
    nextOp ();                                              \
}

#if d_m3HasFloat
d_m3ReinterpretOp (_r0, i32, _fp0, f32)
d_m3ReinterpretOp (_r0, i64, _fp0, f64)
d_m3ReinterpretOp (_fp0, f32, _r0, i32)
d_m3ReinterpretOp (_fp0, f64, _r0, i64)
#endif


d_m3Op  (GetGlobal_s32)
{
    u32 * global = immediate (u32 *);
    slot_store (u32, * global);                   //  printf ("get global: %p %" PRIi64 "\n", global, *global);

    nextOp ();
}


d_m3Op  (GetGlobal_s64)
{
    u64 * global = immediate (u64 *);
    slot_store (u64, * global);                   // printf ("get global: %p %" PRIi64 "\n", global, *global);

    nextOp ();
}


d_m3Op  (SetGlobal_i32)
{
    u32 * global = immediate (u32 *);
    * global = (u32) _r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    nextOp ();
}


d_m3Op  (SetGlobal_i64)
{
    u64 * global = immediate (u64 *);
    * global = (u64) _r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    nextOp ();
}


d_m3Op  (Call)
{
    pc_t callPC                 = immediate (pc_t);
    i32 stackOffset             = immediate (i32);
    IM3Memory memory            = m3MemInfo (_mem);

    m3stack_t sp = _sp + stackOffset;

# if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    m3ret_t r = Call (callPC, sp, _mem, d_m3OpDefaultArgs, d_m3BaseCstr);
# else
    m3ret_t r = Call (callPC, sp, _mem, d_m3OpDefaultArgs);
# endif

    _mem = memory->mallocated;

    if (M3_LIKELY(not r))
    {
#if d_m3EnableLocalRegCaching
        IM3Function caller = immediate (IM3Function);
        M3_RELOAD_LOCAL_REGS (caller);
#endif
        nextOp ();
    }
    else
    {
        pushBacktraceFrame ();
        forwardTrap (r);
    }
}


d_m3Op  (CallIndirect)
{
    u32 tableIndex              = slot (u32);
    IM3Module module            = immediate (IM3Module);
    IM3FuncType type            = immediate (IM3FuncType);
    i32 stackOffset             = immediate (i32);
    IM3Memory memory            = m3MemInfo (_mem);

    m3stack_t sp = _sp + stackOffset;

    m3ret_t r = m3Err_none;

    if (M3_LIKELY(tableIndex < module->table0Size))
    {
        IM3Function function = module->table0 [tableIndex];

        if (M3_LIKELY(function))
        {
            if (M3_LIKELY(type == function->funcType))
            {
                if (M3_UNLIKELY(not function->compiled))
                    r = CompileFunction (function);

                if (M3_LIKELY(not r))
                {

# if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                    r = Call (function->compiled, sp, _mem, d_m3OpDefaultArgs, d_m3BaseCstr);
# else
                    r = Call (function->compiled, sp, _mem, d_m3OpDefaultArgs);
# endif

                    _mem = memory->mallocated;

                    if (M3_LIKELY(not r))
                    {
#if d_m3EnableLocalRegCaching
                        IM3Function caller = immediate (IM3Function);
                        M3_RELOAD_LOCAL_REGS (caller);
#endif
                        nextOpDirect ();
                    }
                    else
                    {
                        pushBacktraceFrame ();
                        forwardTrap (r);
                    }
                }
            }
            else r = m3Err_trapIndirectCallTypeMismatch;
        }
        else r = m3Err_trapTableElementIsNull;
    }
    else r = m3Err_trapTableIndexOutOfRange;

    if (M3_UNLIKELY(r))
        newTrap (r);
    else forwardTrap (r);
}


d_m3Op  (CallRawFunction)
{
    d_m3TracePrepare

    M3ImportContext ctx;

    M3RawCall call = (M3RawCall) (* _pc++);
    ctx.function = immediate (IM3Function);
    ctx.userdata = immediate (void *);
    u64* const sp = ((u64*)_sp);
    IM3Memory memory = m3MemInfo (_mem);

    IM3Runtime runtime = m3MemRuntime(_mem);

#if d_m3EnableStrace
    IM3FuncType ftype = ctx.function->funcType;

    FILE* out = stderr;
    char outbuff[1024];
    char* outp = outbuff;
    char* oute = outbuff+1024;

    outp += snprintf(outp, oute-outp, "%s!%s(", ctx.function->import.moduleUtf8, ctx.function->import.fieldUtf8);

    const int nArgs = ftype->numArgs;
    const int nRets = ftype->numRets;
    u64 * args = sp + nRets;
    for (int i=0; i<nArgs; i++) {
        const int type = ftype->types[nRets + i];
        switch (type) {
        case c_m3Type_i32:  outp += snprintf(outp, oute-outp, "%" PRIi32, *(i32*)(args+i)); break;
        case c_m3Type_i64:  outp += snprintf(outp, oute-outp, "%" PRIi64, *(i64*)(args+i)); break;
        case c_m3Type_f32:  outp += snprintf(outp, oute-outp, "%" PRIf32, *(f32*)(args+i)); break;
        case c_m3Type_f64:  outp += snprintf(outp, oute-outp, "%" PRIf64, *(f64*)(args+i)); break;
        default:            outp += snprintf(outp, oute-outp, "<type %d>", type);         break;
        }
        outp += snprintf(outp, oute-outp, (i < nArgs-1) ? ", " : ")");
    }
# if d_m3EnableStrace >= 2
    outp += snprintf(outp, oute-outp, " { <native> }");
# endif
#endif

    // m3_Call uses runtime->stack to set-up initial exported function stack.
    // Reconfigure the stack to enable recursive invocations of m3_Call.
    // I.e. exported/table function can be called from an impoted function.
    void* stack_backup = runtime->stack;
    runtime->stack = sp;
    m3ret_t possible_trap = call (runtime, &ctx, sp, m3MemData(_mem));
    runtime->stack = stack_backup;

#if d_m3EnableStrace
    if (M3_UNLIKELY(possible_trap)) {
        d_m3TracePrint("%s -> %s", outbuff, (char*)possible_trap);
    } else {
        switch (GetSingleRetType(ftype)) {
        case c_m3Type_none: d_m3TracePrint("%s", outbuff); break;
        case c_m3Type_i32:  d_m3TracePrint("%s = %" PRIi32, outbuff, *(i32*)sp); break;
        case c_m3Type_i64:  d_m3TracePrint("%s = %" PRIi64, outbuff, *(i64*)sp); break;
        case c_m3Type_f32:  d_m3TracePrint("%s = %" PRIf32, outbuff, *(f32*)sp); break;
        case c_m3Type_f64:  d_m3TracePrint("%s = %" PRIf64, outbuff, *(f64*)sp); break;
        }
    }
#endif

    if (M3_UNLIKELY(possible_trap)) {
        _mem = memory->mallocated;
        pushBacktraceFrame ();
    }
    forwardTrap (possible_trap);
}


d_m3Op  (MemSize)
{
    IM3Memory memory            = m3MemInfo (_mem);

    _r0 = memory->numPages;

    nextOp ();
}


d_m3Op  (MemGrow)
{
    IM3Runtime runtime          = m3MemRuntime(_mem);
    IM3Memory memory            = & runtime->memory;

    i32 numPagesToGrow = _r0;
    if (numPagesToGrow >= 0) {
        _r0 = memory->numPages;

        if (M3_LIKELY(numPagesToGrow))
        {
            u32 requiredPages = memory->numPages + numPagesToGrow;

            M3Result r = ResizeMemory (runtime, requiredPages);
            if (r)
                _r0 = -1;

            _mem = memory->mallocated;
        }
    }
    else
    {
        _r0 = -1;
    }

    nextOp ();
}


d_m3Op  (MemCopy)
{
    u32 size = (u32) _r0;
    u64 source = slot (u32);
    u64 destination = slot (u32);

    if (M3_LIKELY(destination + size <= _mem->length))
    {
        if (M3_LIKELY(source + size <= _mem->length))
        {
            u8 * dst = m3MemData (_mem) + destination;
            u8 * src = m3MemData (_mem) + source;
            memmove (dst, src, size);

            nextOp ();
        }
        else d_outOfBoundsMemOp (source, size);
    }
    else d_outOfBoundsMemOp (destination, size);
}


d_m3Op  (MemFill)
{
    u32 size = (u32) _r0;
    u32 byte = slot (u32);
    u64 destination = slot (u32);

    if (M3_LIKELY(destination + size <= _mem->length))
    {
        u8 * mem8 = m3MemData (_mem) + destination;
        memset (mem8, (u8) byte, size);
        nextOp ();
    }
    else d_outOfBoundsMemOp (destination, size);
}


// it's a debate: should the compilation be trigger be the caller or callee page.
// it's a much easier to put it in the caller pager. if it's in the callee, either the entire page
// has be left dangling or it's just a stub that jumps to a newly acquired page.  In Gestalt, I opted
// for the stub approach. Stubbing makes it easier to dynamically free the compilation. You can also
// do both.
d_m3Op  (Compile)
{
    rewrite_op (op_Call);

    IM3Function function        = immediate (IM3Function);

    m3ret_t result = m3Err_none;

    if (M3_UNLIKELY(not function->compiled)) // check to see if function was compiled since this operation was emitted.
        result = CompileFunction (function);

    if (not result)
    {
        // patch up compiled pc and call rewritten op_Call
        * ((void**) --_pc) = (void*) (function->compiled);
        --_pc;
        nextOpDirect ();
    }

    newTrap (result);
}



d_m3Op  (Entry)
{
    d_m3ClearRegisters

    d_m3TracePrepare

    IM3Function function = immediate (IM3Function);
    IM3Memory memory = m3MemInfo (_mem);

#if d_m3SkipStackCheck
    if (true)
#else
    if (M3_LIKELY ((void *) (_sp + function->maxStackSlots) < _mem->maxStack))
#endif
    {
#if defined(DEBUG)
        function->hits++;
#endif
        u8 * stack = (u8 *) ((m3slot_t *) _sp + function->numRetAndArgSlots);

        memset (stack, 0x0, function->numLocalBytes);
        stack += function->numLocalBytes;

        if (function->constants)
        {
            memcpy (stack, function->constants, function->numConstantBytes);
        }

#if d_m3EnableStrace >= 2
        d_m3TracePrint("%s %s {", m3_GetFunctionName(function), SPrintFunctionArgList (function, _sp + function->numRetSlots));
        trace_rt->callDepth++;
#endif

#if d_m3EnableLocalRegCaching
        M3_RELOAD_LOCAL_REGS (function);
#endif

        m3ret_t r = nextOpImpl ();

#if d_m3EnableStrace >= 2
        trace_rt->callDepth--;

        if (r) {
            d_m3TracePrint("} !trap = %s", (char*)r);
        } else {
            int rettype = GetSingleRetType(function->funcType);
            if (rettype != c_m3Type_none) {
                char str [128] = { 0 };
                SPrintArg (str, 127, _sp, rettype);
                d_m3TracePrint("} = %s", str);
            } else {
                d_m3TracePrint("}");
            }
        }
#endif

        if (M3_UNLIKELY(r)) {
            _mem = memory->mallocated;
            fillBacktraceFrame ();
        }
        forwardTrap (r);
    }
    else newTrap (m3Err_trapStackOverflow);
}


d_m3Op  (Loop)
{
    d_m3TracePrepare

    // regs are unused coming into a loop anyway
    // this reduces code size & stack usage
    d_m3ClearRegisters

    m3ret_t r;

#if d_m3EnableLocalRegCaching
    IM3Function function = immediate (IM3Function);
#endif

    IM3Memory memory = m3MemInfo (_mem);

    do
    {
#if d_m3EnableStrace >= 3
        d_m3TracePrint("iter {");
        trace_rt->callDepth++;
#endif

#if d_m3EnableLocalRegCaching
        M3_RELOAD_LOCAL_REGS (function);
#endif
        r = nextOpImpl ();

#if d_m3EnableStrace >= 3
        trace_rt->callDepth--;
        d_m3TracePrint("}");
#endif
        // linear memory pointer needs refreshed here because the block it's looping over
        // can potentially invoke the grow operation.
        _mem = memory->mallocated;
    }
    while (r == _pc);

    forwardTrap (r);
}


d_m3Op  (Branch)
{
    jumpOp (* _pc);
}


d_m3Op  (If_r)
{
    i32 condition = (i32) _r0;

    pc_t elsePC = immediate (pc_t);

    if (condition)
        nextOp ();
    else
        jumpOp (elsePC);
}


d_m3Op  (If_s)
{
    i32 condition = slot (i32);

    pc_t elsePC = immediate (pc_t);

    if (condition)
        nextOp ();
    else
        jumpOp (elsePC);
}


d_m3Op  (BranchTable)
{
    u32 branchIndex = slot (u32);           // branch index is always in a slot
    u32 numTargets  = immediate (u32);

    pc_t * branches = (pc_t *) _pc;

    if (branchIndex > numTargets)
        branchIndex = numTargets; // the default index

    jumpOp (branches [branchIndex]);
}


#define d_m3SetRegisterSetSlot(TYPE, REG) \
d_m3Op  (SetRegister_##TYPE)            \
{                                       \
    REG = slot (TYPE);                  \
    nextOp ();                          \
}                                       \
                                        \
d_m3Op (SetSlot_##TYPE)                 \
{                                       \
    slot_store (TYPE, (TYPE) REG);      \
    nextOp ();                          \
}                                       \
                                        \
d_m3Op (PreserveSetSlot_##TYPE)         \
{                                       \
    const i32 stackOffset = immediate (i32);         \
    const i32 preserveOffset = immediate (i32);      \
                                                    \
    const TYPE oldValue = M3_SLOT_LOAD (TYPE, stackOffset);  \
    M3_SLOT_STORE (TYPE, preserveOffset, oldValue);          \
    M3_SLOT_STORE (TYPE, stackOffset, (TYPE) REG);           \
                                        \
    nextOp ();                          \
}

d_m3SetRegisterSetSlot (i32, _r0)
d_m3SetRegisterSetSlot (i64, _r0)
#if d_m3HasFloat
d_m3SetRegisterSetSlot (f32, _fp0)
d_m3SetRegisterSetSlot (f64, _fp0)
#endif

d_m3Op (CopySlot_32)
{
    const i32 dstOffset = immediate (i32);
    const i32 srcOffset = immediate (i32);

    const u32 value = M3_SLOT_LOAD (u32, srcOffset);
    M3_SLOT_STORE (u32, dstOffset, value);

    nextOp ();
}


d_m3Op (PreserveCopySlot_32)
{
    const i32 destOffset = immediate (i32);
    const i32 srcOffset = immediate (i32);
    const i32 preserveOffset = immediate (i32);

    const u32 oldValue = M3_SLOT_LOAD (u32, destOffset);
    M3_SLOT_STORE (u32, preserveOffset, oldValue);

    const u32 value = M3_SLOT_LOAD (u32, srcOffset);
    M3_SLOT_STORE (u32, destOffset, value);

    nextOp ();
}


d_m3Op (CopySlot_64)
{
    const i32 dstOffset = immediate (i32);
    const i32 srcOffset = immediate (i32);

    const u64 value = M3_SLOT_LOAD (u64, srcOffset);
    M3_SLOT_STORE (u64, dstOffset, value);    // printf ("copy: %p <- %" PRIi64 "\n", dst, * dst);

    nextOp ();
}


d_m3Op (PreserveCopySlot_64)
{
    const i32 destOffset = immediate (i32);
    const i32 srcOffset = immediate (i32);
    const i32 preserveOffset = immediate (i32);

    const u64 oldValue = M3_SLOT_LOAD (u64, destOffset);
    M3_SLOT_STORE (u64, preserveOffset, oldValue);

    const u64 value = M3_SLOT_LOAD (u64, srcOffset);
    M3_SLOT_STORE (u64, destOffset, value);

    nextOp ();
}


#if d_m3EnableOpTracing
//--------------------------------------------------------------------------------------------------------
d_m3Op  (DumpStack)
{
    u32 opcodeIndex         = immediate (u32);
    u32 stackHeight         = immediate (u32);
    IM3Function function    = immediate (IM3Function);

    cstr_t funcName = (function) ? m3_GetFunctionName(function) : "";

    printf (" %4d ", opcodeIndex);
    printf (" %-25s     r0: 0x%016" PRIx64 "  i:%" PRIi64 "  u:%" PRIu64 "\n", funcName, _r0, _r0, _r0);
#if d_m3HasFloat
    printf ("                                    fp0: %" PRIf64 "\n", _fp0);
#endif
    m3stack_t sp = _sp;

    for (u32 i = 0; i < stackHeight; ++i)
    {
        cstr_t kind = "";

        printf ("%p  %5s  %2d: 0x%" PRIx64 "  i:%" PRIi64 "\n", sp, kind, i, (u64) *(sp), (i64) *(sp));

        ++sp;
    }
    printf ("---------------------------------------------------------------------------------------------------------\n");

    nextOpDirect();
}
#endif


#define d_m3Select_i(TYPE, REG)                 \
d_m3Op  (Select_##TYPE##_rss)                   \
{                                               \
    i32 condition = (i32) _r0;                  \
                                                \
    TYPE operand2 = slot (TYPE);                \
    TYPE operand1 = slot (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    nextOp ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_srs)                   \
{                                               \
    i32 condition = slot (i32);                 \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = slot (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    nextOp ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_ssr)                   \
{                                               \
    i32 condition = slot (i32);                 \
                                                \
    TYPE operand2 = slot (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    nextOp ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_sss)                   \
{                                               \
    i32 condition = slot (i32);                 \
                                                \
    TYPE operand2 = slot (TYPE);                \
    TYPE operand1 = slot (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    nextOp ();                                  \
}


d_m3Select_i (i32, _r0)
d_m3Select_i (i64, _r0)


#define d_m3Select_f(TYPE, REG, LABEL, SELECTOR)  \
d_m3Op  (Select_##TYPE##_##LABEL##ss)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = slot (TYPE);                \
    TYPE operand1 = slot (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    nextOp ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##rs)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = slot (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    nextOp ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##sr)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = slot (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    nextOp ();                                  \
}

#if d_m3HasFloat
d_m3Select_f (f32, _fp0, r, _r0)
d_m3Select_f (f32, _fp0, s, slot (i32))

d_m3Select_f (f64, _fp0, r, _r0)
d_m3Select_f (f64, _fp0, s, slot (i32))
#endif

d_m3Op  (Return)
{
    m3StackCheck();
    return m3Err_none;
}


d_m3Op  (BranchIf_r)
{
    i32 condition   = (i32) _r0;
    pc_t branch     = immediate (pc_t);

    if (condition)
    {
        jumpOp (branch);
    }
    else nextOp ();
}


d_m3Op  (BranchIf_s)
{
    i32 condition   = slot (i32);
    pc_t branch     = immediate (pc_t);

    if (condition)
    {
        jumpOp (branch);
    }
    else nextOp ();
}


d_m3Op  (BranchIfPrologue_r)
{
    i32 condition   = (i32) _r0;
    pc_t branch     = immediate (pc_t);

    if (condition)
    {
        // this is the "prologue" that ends with
        // a plain branch to the actual target
        nextOp ();
    }
    else jumpOp (branch); // jump over the prologue
}


d_m3Op  (BranchIfPrologue_s)
{
    i32 condition   = slot (i32);
    pc_t branch     = immediate (pc_t);

    if (condition)
    {
        nextOp ();
    }
    else jumpOp (branch);
}


d_m3Op  (ContinueLoop)
{
    m3StackCheck();

    // TODO: this is where execution can "escape" the M3 code and callback to the client / fiber switch
    // OR it can go in the Loop operation. I think it's best to do here. adding code to the loop operation
    // has the potential to increase its native-stack usage. (don't forget ContinueLoopIf too.)

    void * loopId = immediate (void *);
    return loopId;
}


d_m3Op  (ContinueLoopIf)
{
    i32 condition = (i32) _r0;
    void * loopId = immediate (void *);

    if (condition)
    {
        return loopId;
    }
    else nextOp ();
}


d_m3Op  (Const32)
{
    u32 value = * (u32 *)_pc++;
    slot_store (u32, value);
    nextOp ();
}


d_m3Op  (Const64)
{
    u64 value = * (u64 *)_pc;
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;
    slot_store (u64, value);
    nextOp ();
}

d_m3Op  (Unsupported)
{
    newTrap ("unsupported instruction executed");
}

d_m3Op  (Unreachable)
{
    m3StackCheck();
    newTrap (m3Err_trapUnreachable);
}


d_m3Op  (End)
{
    m3StackCheck();
    return m3Err_none;
}


d_m3Op  (SetGlobal_s32)
{
    u32 * global = immediate (u32 *);
    * global = slot (u32);

    nextOp ();
}


d_m3Op  (SetGlobal_s64)
{
    u64 * global = immediate (u64 *);
    * global = slot (u64);

    nextOp ();
}

#if d_m3HasFloat
d_m3Op  (SetGlobal_f32)
{
    f32 * global = immediate (f32 *);
    * global = _fp0;

    nextOp ();
}


d_m3Op  (SetGlobal_f64)
{
    f64 * global = immediate (f64 *);
    * global = _fp0;

    nextOp ();
}
#endif


#if d_m3SkipMemoryBoundsCheck
#  define m3MemCheck(x) true
#else
#  define m3MemCheck(x) M3_LIKELY(x)
#endif

// memcpy here is to support non-aligned access on some platforms.

#define d_m3Load(REG,DEST_TYPE,SRC_TYPE)                \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u32 offset = immediate (u32);                       \
    u64 operand = (u32) _r0;                            \
    operand += offset;                                  \
                                                        \
    if (m3MemCheck(                                     \
        operand + sizeof (SRC_TYPE) <= _mem->length     \
    )) {                                                \
        {                                               \
            u8* src8 = m3MemData(_mem) + operand;       \
            SRC_TYPE value;                             \
            memcpy(&value, src8, sizeof(value));        \
            M3_BSWAP_##SRC_TYPE(value);                 \
            REG = (DEST_TYPE)value;                     \
            d_m3TraceLoad(DEST_TYPE, operand, REG);     \
        }                                               \
        nextOp ();                                      \
    } else d_outOfBounds;                               \
}                                                       \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u64 operand = slot (u32);                           \
    u32 offset = immediate (u32);                       \
    operand += offset;                                  \
                                                        \
    if (m3MemCheck(                                     \
        operand + sizeof (SRC_TYPE) <= _mem->length     \
    )) {                                                \
        {                                               \
            u8* src8 = m3MemData(_mem) + operand;       \
            SRC_TYPE value;                             \
            memcpy(&value, src8, sizeof(value));        \
            M3_BSWAP_##SRC_TYPE(value);                 \
            REG = (DEST_TYPE)value;                     \
            d_m3TraceLoad(DEST_TYPE, operand, REG);     \
        }                                               \
        nextOp ();                                      \
    } else d_outOfBounds;                               \
}

//  printf ("get: %d -> %d\n", operand + offset, (i64) REG);


#define d_m3Load_i(DEST_TYPE, SRC_TYPE) d_m3Load(_r0, DEST_TYPE, SRC_TYPE)
#define d_m3Load_f(DEST_TYPE, SRC_TYPE) d_m3Load(_fp0, DEST_TYPE, SRC_TYPE)

#if d_m3HasFloat
d_m3Load_f (f32, f32);
d_m3Load_f (f64, f64);
#endif

d_m3Load_i (i32, i8);
d_m3Load_i (i32, u8);
d_m3Load_i (i32, i16);
d_m3Load_i (i32, u16);
d_m3Load_i (i32, i32);

d_m3Load_i (i64, i8);
d_m3Load_i (i64, u8);
d_m3Load_i (i64, i16);
d_m3Load_i (i64, u16);
d_m3Load_i (i64, i32);
d_m3Load_i (i64, u32);
d_m3Load_i (i64, i64);

#define d_m3Store(REG, SRC_TYPE, DEST_TYPE)             \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_rs)             \
{                                                       \
    d_m3TracePrepare                                    \
    u64 operand = slot (u32);                           \
    u32 offset = immediate (u32);                       \
    operand += offset;                                  \
                                                        \
    if (m3MemCheck(                                     \
        operand + sizeof (DEST_TYPE) <= _mem->length    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, operand, REG);     \
            u8* mem8 = m3MemData(_mem) + operand;       \
            DEST_TYPE val = (DEST_TYPE) REG;            \
            M3_BSWAP_##DEST_TYPE(val);                  \
            memcpy(mem8, &val, sizeof(val));            \
        }                                               \
        nextOp ();                                      \
    } else d_outOfBounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_sr)             \
{                                                       \
    d_m3TracePrepare                                    \
    const SRC_TYPE value = slot (SRC_TYPE);             \
    u64 operand = (u32) _r0;                            \
    u32 offset = immediate (u32);                       \
    operand += offset;                                  \
                                                        \
    if (m3MemCheck(                                     \
        operand + sizeof (DEST_TYPE) <= _mem->length    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, operand, value);   \
            u8* mem8 = m3MemData(_mem) + operand;       \
            DEST_TYPE val = (DEST_TYPE) value;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            memcpy(mem8, &val, sizeof(val));            \
        }                                               \
        nextOp ();                                      \
    } else d_outOfBounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_ss)             \
{                                                       \
    d_m3TracePrepare                                    \
    const SRC_TYPE value = slot (SRC_TYPE);             \
    u64 operand = slot (u32);                           \
    u32 offset = immediate (u32);                       \
    operand += offset;                                  \
                                                        \
    if (m3MemCheck(                                     \
        operand + sizeof (DEST_TYPE) <= _mem->length    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, operand, value);   \
            u8* mem8 = m3MemData(_mem) + operand;       \
            DEST_TYPE val = (DEST_TYPE) value;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            memcpy(mem8, &val, sizeof(val));            \
        }                                               \
        nextOp ();                                      \
    } else d_outOfBounds;                               \
}

// both operands can be in regs when storing a float
#define d_m3StoreFp(REG, TYPE)                          \
d_m3Op  (TYPE##_Store_##TYPE##_rr)                      \
{                                                       \
    d_m3TracePrepare                                    \
    u64 operand = (u32) _r0;                            \
    u32 offset = immediate (u32);                       \
    operand += offset;                                  \
                                                        \
    if (m3MemCheck(                                     \
        operand + sizeof (TYPE) <= _mem->length         \
    )) {                                                \
        {                                               \
            d_m3TraceStore(TYPE, operand, REG);         \
            u8* mem8 = m3MemData(_mem) + operand;       \
            TYPE val = (TYPE) REG;                      \
            M3_BSWAP_##TYPE(val);                       \
            memcpy(mem8, &val, sizeof(val));            \
        }                                               \
        nextOp ();                                      \
    } else d_outOfBounds;                               \
}


#define d_m3Store_i(SRC_TYPE, DEST_TYPE) d_m3Store(_r0, SRC_TYPE, DEST_TYPE)
#define d_m3Store_f(SRC_TYPE, DEST_TYPE) d_m3Store(_fp0, SRC_TYPE, DEST_TYPE) d_m3StoreFp (_fp0, SRC_TYPE);

#if d_m3HasFloat
d_m3Store_f (f32, f32)
d_m3Store_f (f64, f64)
#endif

d_m3Store_i (i32, u8)
d_m3Store_i (i32, i16)
d_m3Store_i (i32, i32)

d_m3Store_i (i64, u8)
d_m3Store_i (i64, i16)
d_m3Store_i (i64, i32)
d_m3Store_i (i64, i64)

#undef m3MemCheck


//---------------------------------------------------------------------------------------------------------------------
// debug/profiling
//---------------------------------------------------------------------------------------------------------------------
#if d_m3EnableOpTracing
d_m3RetSig  debugOp  (d_m3OpSig, cstr_t i_opcode)
{
    char name [100];
    strcpy (name, strstr (i_opcode, "op_") + 3);
    char * bracket = strstr (name, "(");
    if (bracket) {
        *bracket  = 0;
    }

    puts (name);
    nextOpDirect();
}
# endif

# if d_m3EnableOpProfiling
d_m3RetSig  profileOp  (d_m3OpSig, cstr_t i_operationName)
{
    ProfileHit (i_operationName);

    nextOpDirect();
}
# endif

d_m3EndExternC

#endif // m3_exec_h
