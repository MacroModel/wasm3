//
//  m3_fp_edge_test.c
//
//  Floating-point edge-case regression tests for wasm3.
//  Focus: NaN comparisons, signed zero, and float->int conversion traps/saturations.
//
//  The embedded wasm binary was generated from a small .wat using:
//    wat2wasm m3_fp_edge.wat -o m3_fp_edge.wasm
//    xxd -i m3_fp_edge.wasm
//

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <fenv.h>

#include "wasm3.h"
#include "m3_env.h"

static const uint8_t wasm_fp_edge_wasm[] = {
  0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x24, 0x07, 0x60,
  0x01, 0x7f, 0x01, 0x7f, 0x60, 0x01, 0x7e, 0x01, 0x7f, 0x60, 0x00, 0x01,
  0x7f, 0x60, 0x00, 0x01, 0x7e, 0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x60,
  0x02, 0x7e, 0x7e, 0x01, 0x7e, 0x60, 0x01, 0x7e, 0x01, 0x7e, 0x03, 0x1f,
  0x1e, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05,
  0x05, 0x00, 0x00, 0x06, 0x06, 0x04, 0x04, 0x00, 0x05, 0x05, 0x06, 0x00,
  0x00, 0x00, 0x00, 0x06, 0x06, 0x06, 0x06, 0x07, 0xe9, 0x03, 0x1e, 0x0b,
  0x66, 0x33, 0x32, 0x5f, 0x73, 0x65, 0x6c, 0x66, 0x5f, 0x65, 0x71, 0x00,
  0x00, 0x0b, 0x66, 0x33, 0x32, 0x5f, 0x73, 0x65, 0x6c, 0x66, 0x5f, 0x6e,
  0x65, 0x00, 0x01, 0x0b, 0x66, 0x36, 0x34, 0x5f, 0x73, 0x65, 0x6c, 0x66,
  0x5f, 0x65, 0x71, 0x00, 0x02, 0x0b, 0x66, 0x36, 0x34, 0x5f, 0x73, 0x65,
  0x6c, 0x66, 0x5f, 0x6e, 0x65, 0x00, 0x03, 0x0c, 0x66, 0x33, 0x32, 0x5f,
  0x6d, 0x69, 0x6e, 0x5f, 0x7a, 0x65, 0x72, 0x6f, 0x00, 0x04, 0x0c, 0x66,
  0x33, 0x32, 0x5f, 0x6d, 0x61, 0x78, 0x5f, 0x7a, 0x65, 0x72, 0x6f, 0x00,
  0x05, 0x0c, 0x66, 0x36, 0x34, 0x5f, 0x6d, 0x69, 0x6e, 0x5f, 0x7a, 0x65,
  0x72, 0x6f, 0x00, 0x06, 0x0c, 0x66, 0x36, 0x34, 0x5f, 0x6d, 0x61, 0x78,
  0x5f, 0x7a, 0x65, 0x72, 0x6f, 0x00, 0x07, 0x0c, 0x66, 0x33, 0x32, 0x5f,
  0x6d, 0x69, 0x6e, 0x5f, 0x62, 0x69, 0x74, 0x73, 0x00, 0x08, 0x0c, 0x66,
  0x33, 0x32, 0x5f, 0x6d, 0x61, 0x78, 0x5f, 0x62, 0x69, 0x74, 0x73, 0x00,
  0x09, 0x0c, 0x66, 0x36, 0x34, 0x5f, 0x6d, 0x69, 0x6e, 0x5f, 0x62, 0x69,
  0x74, 0x73, 0x00, 0x0a, 0x0c, 0x66, 0x36, 0x34, 0x5f, 0x6d, 0x61, 0x78,
  0x5f, 0x62, 0x69, 0x74, 0x73, 0x00, 0x0b, 0x0c, 0x66, 0x33, 0x32, 0x5f,
  0x6e, 0x65, 0x67, 0x5f, 0x62, 0x69, 0x74, 0x73, 0x00, 0x0c, 0x0c, 0x66,
  0x33, 0x32, 0x5f, 0x61, 0x62, 0x73, 0x5f, 0x62, 0x69, 0x74, 0x73, 0x00,
  0x0d, 0x0c, 0x66, 0x36, 0x34, 0x5f, 0x6e, 0x65, 0x67, 0x5f, 0x62, 0x69,
  0x74, 0x73, 0x00, 0x0e, 0x0c, 0x66, 0x36, 0x34, 0x5f, 0x61, 0x62, 0x73,
  0x5f, 0x62, 0x69, 0x74, 0x73, 0x00, 0x0f, 0x0c, 0x66, 0x33, 0x32, 0x5f,
  0x61, 0x64, 0x64, 0x5f, 0x62, 0x69, 0x74, 0x73, 0x00, 0x10, 0x0c, 0x66,
  0x33, 0x32, 0x5f, 0x64, 0x69, 0x76, 0x5f, 0x62, 0x69, 0x74, 0x73, 0x00,
  0x11, 0x0d, 0x66, 0x33, 0x32, 0x5f, 0x73, 0x71, 0x72, 0x74, 0x5f, 0x62,
  0x69, 0x74, 0x73, 0x00, 0x12, 0x0c, 0x66, 0x36, 0x34, 0x5f, 0x61, 0x64,
  0x64, 0x5f, 0x62, 0x69, 0x74, 0x73, 0x00, 0x13, 0x0c, 0x66, 0x36, 0x34,
  0x5f, 0x64, 0x69, 0x76, 0x5f, 0x62, 0x69, 0x74, 0x73, 0x00, 0x14, 0x0d,
  0x66, 0x36, 0x34, 0x5f, 0x73, 0x71, 0x72, 0x74, 0x5f, 0x62, 0x69, 0x74,
  0x73, 0x00, 0x15, 0x0f, 0x69, 0x33, 0x32, 0x5f, 0x74, 0x72, 0x75, 0x6e,
  0x63, 0x5f, 0x66, 0x33, 0x32, 0x5f, 0x73, 0x00, 0x16, 0x0f, 0x69, 0x33,
  0x32, 0x5f, 0x74, 0x72, 0x75, 0x6e, 0x63, 0x5f, 0x66, 0x33, 0x32, 0x5f,
  0x75, 0x00, 0x17, 0x13, 0x69, 0x33, 0x32, 0x5f, 0x74, 0x72, 0x75, 0x6e,
  0x63, 0x5f, 0x73, 0x61, 0x74, 0x5f, 0x66, 0x33, 0x32, 0x5f, 0x73, 0x00,
  0x18, 0x13, 0x69, 0x33, 0x32, 0x5f, 0x74, 0x72, 0x75, 0x6e, 0x63, 0x5f,
  0x73, 0x61, 0x74, 0x5f, 0x66, 0x33, 0x32, 0x5f, 0x75, 0x00, 0x19, 0x0f,
  0x69, 0x36, 0x34, 0x5f, 0x74, 0x72, 0x75, 0x6e, 0x63, 0x5f, 0x66, 0x36,
  0x34, 0x5f, 0x73, 0x00, 0x1a, 0x0f, 0x69, 0x36, 0x34, 0x5f, 0x74, 0x72,
  0x75, 0x6e, 0x63, 0x5f, 0x66, 0x36, 0x34, 0x5f, 0x75, 0x00, 0x1b, 0x13,
  0x69, 0x36, 0x34, 0x5f, 0x74, 0x72, 0x75, 0x6e, 0x63, 0x5f, 0x73, 0x61,
  0x74, 0x5f, 0x66, 0x36, 0x34, 0x5f, 0x73, 0x00, 0x1c, 0x13, 0x69, 0x36,
  0x34, 0x5f, 0x74, 0x72, 0x75, 0x6e, 0x63, 0x5f, 0x73, 0x61, 0x74, 0x5f,
  0x66, 0x36, 0x34, 0x5f, 0x75, 0x00, 0x1d, 0x0a, 0xc5, 0x02, 0x1e, 0x0c,
  0x01, 0x01, 0x7d, 0x20, 0x00, 0xbe, 0x22, 0x01, 0x20, 0x01, 0x5b, 0x0b,
  0x0c, 0x01, 0x01, 0x7d, 0x20, 0x00, 0xbe, 0x22, 0x01, 0x20, 0x01, 0x5c,
  0x0b, 0x0c, 0x01, 0x01, 0x7c, 0x20, 0x00, 0xbf, 0x22, 0x01, 0x20, 0x01,
  0x61, 0x0b, 0x0c, 0x01, 0x01, 0x7c, 0x20, 0x00, 0xbf, 0x22, 0x01, 0x20,
  0x01, 0x62, 0x0b, 0x0e, 0x00, 0x43, 0x00, 0x00, 0x00, 0x80, 0x43, 0x00,
  0x00, 0x00, 0x00, 0x96, 0xbc, 0x0b, 0x0e, 0x00, 0x43, 0x00, 0x00, 0x00,
  0x80, 0x43, 0x00, 0x00, 0x00, 0x00, 0x97, 0xbc, 0x0b, 0x16, 0x00, 0x44,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x44, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0xa4, 0xbd, 0x0b, 0x16, 0x00, 0x44, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x44, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xa5, 0xbd, 0x0b, 0x0a, 0x00, 0x20, 0x00, 0xbe,
  0x20, 0x01, 0xbe, 0x96, 0xbc, 0x0b, 0x0a, 0x00, 0x20, 0x00, 0xbe, 0x20,
  0x01, 0xbe, 0x97, 0xbc, 0x0b, 0x0a, 0x00, 0x20, 0x00, 0xbf, 0x20, 0x01,
  0xbf, 0xa4, 0xbd, 0x0b, 0x0a, 0x00, 0x20, 0x00, 0xbf, 0x20, 0x01, 0xbf,
  0xa5, 0xbd, 0x0b, 0x07, 0x00, 0x20, 0x00, 0xbe, 0x8c, 0xbc, 0x0b, 0x07,
  0x00, 0x20, 0x00, 0xbe, 0x8b, 0xbc, 0x0b, 0x07, 0x00, 0x20, 0x00, 0xbf,
  0x9a, 0xbd, 0x0b, 0x07, 0x00, 0x20, 0x00, 0xbf, 0x99, 0xbd, 0x0b, 0x0a,
  0x00, 0x20, 0x00, 0xbe, 0x20, 0x01, 0xbe, 0x92, 0xbc, 0x0b, 0x0a, 0x00,
  0x20, 0x00, 0xbe, 0x20, 0x01, 0xbe, 0x95, 0xbc, 0x0b, 0x07, 0x00, 0x20,
  0x00, 0xbe, 0x91, 0xbc, 0x0b, 0x0a, 0x00, 0x20, 0x00, 0xbf, 0x20, 0x01,
  0xbf, 0xa0, 0xbd, 0x0b, 0x0a, 0x00, 0x20, 0x00, 0xbf, 0x20, 0x01, 0xbf,
  0xa3, 0xbd, 0x0b, 0x07, 0x00, 0x20, 0x00, 0xbf, 0x9f, 0xbd, 0x0b, 0x06,
  0x00, 0x20, 0x00, 0xbe, 0xa8, 0x0b, 0x06, 0x00, 0x20, 0x00, 0xbe, 0xa9,
  0x0b, 0x07, 0x00, 0x20, 0x00, 0xbe, 0xfc, 0x00, 0x0b, 0x07, 0x00, 0x20,
  0x00, 0xbe, 0xfc, 0x01, 0x0b, 0x06, 0x00, 0x20, 0x00, 0xbf, 0xb0, 0x0b,
  0x06, 0x00, 0x20, 0x00, 0xbf, 0xb1, 0x0b, 0x07, 0x00, 0x20, 0x00, 0xbf,
  0xfc, 0x06, 0x0b, 0x07, 0x00, 0x20, 0x00, 0xbf, 0xfc, 0x07, 0x0b
};

static const uint32_t wasm_fp_edge_wasm_len = (uint32_t) sizeof(wasm_fp_edge_wasm);

static int g_failures = 0;

static void failf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "FAIL: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    g_failures++;
}

static bool is_nan_u32_f32(uint32_t bits) {
    return ((bits & 0x7f800000u) == 0x7f800000u) && ((bits & 0x007fffffu) != 0);
}

static bool is_nan_u64_f64(uint64_t bits) {
    return ((bits & 0x7ff0000000000000ULL) == 0x7ff0000000000000ULL) &&
           ((bits & 0x000fffffffffffffULL) != 0);
}

static void expect_u32_eq(const char* name, uint32_t got, uint32_t expected) {
    if (got != expected) {
        failf("%s: got=0x%08" PRIx32 " expected=0x%08" PRIx32, name, got, expected);
    }
}

static void expect_u64_eq(const char* name, uint64_t got, uint64_t expected) {
    if (got != expected) {
        failf("%s: got=0x%016" PRIx64 " expected=0x%016" PRIx64, name, got, expected);
    }
}

static void expect_u32_nan(const char* name, uint32_t got) {
    if (!is_nan_u32_f32(got)) {
        failf("%s: got=0x%08" PRIx32 " expected=NaN(f32)", name, got);
    }
}

static void expect_u64_nan(const char* name, uint64_t got) {
    if (!is_nan_u64_f64(got)) {
        failf("%s: got=0x%016" PRIx64 " expected=NaN(f64)", name, got);
    }
}

static void expect_m3_ok(const char* where, M3Result res) {
    if (res != m3Err_none) {
        failf("%s: unexpected error: %s", where, res ? res : "(null)");
    }
}

static void expect_m3_err_eq(const char* where, M3Result res, M3Result expected) {
    if (res != expected) {
        failf("%s: got error=%s expected=%s", where, res ? res : "(null)", expected ? expected : "(null)");
    }
}

static IM3Function find_fn(IM3Runtime runtime, const char* name) {
    IM3Function fn = NULL;
    M3Result res = m3_FindFunction(&fn, runtime, name);
    if (res != m3Err_none || !fn) {
        failf("m3_FindFunction(%s) failed: %s", name, res ? res : "(null)");
        return NULL;
    }
    return fn;
}

int main(void) {
    (void)fesetround(FE_TONEAREST);

    IM3Environment env = m3_NewEnvironment();
    if (!env) {
        fprintf(stderr, "FATAL: m3_NewEnvironment failed\n");
        return 2;
    }

    IM3Runtime runtime = m3_NewRuntime(env, 64 * 1024, NULL);
    if (!runtime) {
        fprintf(stderr, "FATAL: m3_NewRuntime failed\n");
        m3_FreeEnvironment(env);
        return 2;
    }

    IM3Module module = NULL;
    M3Result res = m3_ParseModule(env, &module, wasm_fp_edge_wasm, wasm_fp_edge_wasm_len);
    if (res != m3Err_none) {
        fprintf(stderr, "FATAL: m3_ParseModule failed: %s\n", res);
        m3_FreeRuntime(runtime);
        m3_FreeEnvironment(env);
        return 2;
    }

    res = m3_LoadModule(runtime, module);
    if (res != m3Err_none) {
        fprintf(stderr, "FATAL: m3_LoadModule failed: %s\n", res);
        m3_FreeRuntime(runtime);
        m3_FreeEnvironment(env);
        return 2;
    }

    const uint32_t f32_qnan = 0x7fc00000u;
    const uint32_t f32_snan = 0x7f800001u;
    const uint32_t f32_pzero = 0x00000000u;
    const uint32_t f32_nzero = 0x80000000u;
    const uint32_t f32_pinf = 0x7f800000u;
    const uint32_t f32_ninf = 0xff800000u;
    const uint32_t f32_neg_one = 0xbf800000u;
    const uint32_t f32_i32_min = 0xcf000000u;   // -2147483648.0f
    const uint32_t f32_i32_2p31 = 0x4f000000u;  //  2147483648.0f

    const uint64_t f64_qnan = 0x7ff8000000000000ULL;
    const uint64_t f64_snan = 0x7ff0000000000001ULL;
    const uint64_t f64_pzero = 0x0000000000000000ULL;
    const uint64_t f64_nzero = 0x8000000000000000ULL;
    const uint64_t f64_pinf = 0x7ff0000000000000ULL;
    const uint64_t f64_ninf = 0xfff0000000000000ULL;
    const uint64_t f64_i64_min = 0xc3e0000000000000ULL;   // -9223372036854775808.0
    const uint64_t f64_i64_2p63 = 0x43e0000000000000ULL;  //  9223372036854775808.0

    // NaN comparisons (these are the first things that usually break under -ffast-math)
    {
        IM3Function f32_eq = find_fn(runtime, "f32_self_eq");
        IM3Function f32_ne = find_fn(runtime, "f32_self_ne");
        IM3Function f64_eq = find_fn(runtime, "f64_self_eq");
        IM3Function f64_ne = find_fn(runtime, "f64_self_ne");

        uint32_t out = 0;
        res = m3_CallV(f32_eq, f32_qnan); expect_m3_ok("call f32_self_eq(qNaN)", res);
        m3_GetResultsV(f32_eq, &out); expect_u32_eq("f32_self_eq(qNaN)", out, 0);
        res = m3_CallV(f32_ne, f32_qnan); expect_m3_ok("call f32_self_ne(qNaN)", res);
        m3_GetResultsV(f32_ne, &out); expect_u32_eq("f32_self_ne(qNaN)", out, 1);

        res = m3_CallV(f32_eq, f32_snan); expect_m3_ok("call f32_self_eq(sNaN)", res);
        m3_GetResultsV(f32_eq, &out); expect_u32_eq("f32_self_eq(sNaN)", out, 0);
        res = m3_CallV(f32_ne, f32_snan); expect_m3_ok("call f32_self_ne(sNaN)", res);
        m3_GetResultsV(f32_ne, &out); expect_u32_eq("f32_self_ne(sNaN)", out, 1);

        uint32_t out32 = 0;
        res = m3_CallV(f32_eq, 0x3f800000u); expect_m3_ok("call f32_self_eq(1.0)", res);
        m3_GetResultsV(f32_eq, &out32); expect_u32_eq("f32_self_eq(1.0)", out32, 1);
        res = m3_CallV(f32_ne, 0x3f800000u); expect_m3_ok("call f32_self_ne(1.0)", res);
        m3_GetResultsV(f32_ne, &out32); expect_u32_eq("f32_self_ne(1.0)", out32, 0);

        res = m3_CallV(f64_eq, f64_qnan); expect_m3_ok("call f64_self_eq(qNaN)", res);
        m3_GetResultsV(f64_eq, &out32); expect_u32_eq("f64_self_eq(qNaN)", out32, 0);
        res = m3_CallV(f64_ne, f64_qnan); expect_m3_ok("call f64_self_ne(qNaN)", res);
        m3_GetResultsV(f64_ne, &out32); expect_u32_eq("f64_self_ne(qNaN)", out32, 1);

        res = m3_CallV(f64_eq, f64_snan); expect_m3_ok("call f64_self_eq(sNaN)", res);
        m3_GetResultsV(f64_eq, &out32); expect_u32_eq("f64_self_eq(sNaN)", out32, 0);
        res = m3_CallV(f64_ne, f64_snan); expect_m3_ok("call f64_self_ne(sNaN)", res);
        m3_GetResultsV(f64_ne, &out32); expect_u32_eq("f64_self_ne(sNaN)", out32, 1);
    }

    // Signed zero: min/max and unary ops
    {
        IM3Function f32_min0 = find_fn(runtime, "f32_min_zero");
        IM3Function f32_max0 = find_fn(runtime, "f32_max_zero");
        IM3Function f64_min0 = find_fn(runtime, "f64_min_zero");
        IM3Function f64_max0 = find_fn(runtime, "f64_max_zero");
        IM3Function f32_min = find_fn(runtime, "f32_min_bits");
        IM3Function f32_max = find_fn(runtime, "f32_max_bits");
        IM3Function f64_min = find_fn(runtime, "f64_min_bits");
        IM3Function f64_max = find_fn(runtime, "f64_max_bits");
        IM3Function f32_neg = find_fn(runtime, "f32_neg_bits");
        IM3Function f32_abs = find_fn(runtime, "f32_abs_bits");
        IM3Function f64_neg = find_fn(runtime, "f64_neg_bits");
        IM3Function f64_abs = find_fn(runtime, "f64_abs_bits");

        uint32_t out32 = 0;
        uint64_t out64 = 0;

        res = m3_CallV(f32_min0); expect_m3_ok("call f32_min_zero", res);
        m3_GetResultsV(f32_min0, &out32); expect_u32_eq("f32_min_zero", out32, f32_nzero);

        res = m3_CallV(f32_max0); expect_m3_ok("call f32_max_zero", res);
        m3_GetResultsV(f32_max0, &out32); expect_u32_eq("f32_max_zero", out32, f32_pzero);

        res = m3_CallV(f64_min0); expect_m3_ok("call f64_min_zero", res);
        m3_GetResultsV(f64_min0, &out64); expect_u64_eq("f64_min_zero", out64, f64_nzero);

        res = m3_CallV(f64_max0); expect_m3_ok("call f64_max_zero", res);
        m3_GetResultsV(f64_max0, &out64); expect_u64_eq("f64_max_zero", out64, f64_pzero);

        // NaN handling in min/max: if either operand is NaN, result must be NaN (payload is not specified).
        const uint32_t f32_one = 0x3f800000u;
        const uint64_t f64_one = 0x3ff0000000000000ULL;

        res = m3_CallV(f32_min, f32_qnan, f32_one); expect_m3_ok("call f32_min_bits(NaN,1.0)", res);
        m3_GetResultsV(f32_min, &out32); expect_u32_nan("f32_min_bits(NaN,1.0)", out32);
        res = m3_CallV(f32_min, f32_one, f32_qnan); expect_m3_ok("call f32_min_bits(1.0,NaN)", res);
        m3_GetResultsV(f32_min, &out32); expect_u32_nan("f32_min_bits(1.0,NaN)", out32);

        res = m3_CallV(f32_max, f32_qnan, f32_one); expect_m3_ok("call f32_max_bits(NaN,1.0)", res);
        m3_GetResultsV(f32_max, &out32); expect_u32_nan("f32_max_bits(NaN,1.0)", out32);
        res = m3_CallV(f32_max, f32_one, f32_qnan); expect_m3_ok("call f32_max_bits(1.0,NaN)", res);
        m3_GetResultsV(f32_max, &out32); expect_u32_nan("f32_max_bits(1.0,NaN)", out32);

        res = m3_CallV(f64_min, f64_qnan, f64_one); expect_m3_ok("call f64_min_bits(NaN,1.0)", res);
        m3_GetResultsV(f64_min, &out64); expect_u64_nan("f64_min_bits(NaN,1.0)", out64);
        res = m3_CallV(f64_min, f64_one, f64_qnan); expect_m3_ok("call f64_min_bits(1.0,NaN)", res);
        m3_GetResultsV(f64_min, &out64); expect_u64_nan("f64_min_bits(1.0,NaN)", out64);

        res = m3_CallV(f64_max, f64_qnan, f64_one); expect_m3_ok("call f64_max_bits(NaN,1.0)", res);
        m3_GetResultsV(f64_max, &out64); expect_u64_nan("f64_max_bits(NaN,1.0)", out64);
        res = m3_CallV(f64_max, f64_one, f64_qnan); expect_m3_ok("call f64_max_bits(1.0,NaN)", res);
        m3_GetResultsV(f64_max, &out64); expect_u64_nan("f64_max_bits(1.0,NaN)", out64);

        res = m3_CallV(f32_neg, f32_pzero); expect_m3_ok("call f32_neg_bits(+0)", res);
        m3_GetResultsV(f32_neg, &out32); expect_u32_eq("f32_neg_bits(+0)", out32, f32_nzero);

        res = m3_CallV(f32_abs, f32_nzero); expect_m3_ok("call f32_abs_bits(-0)", res);
        m3_GetResultsV(f32_abs, &out32); expect_u32_eq("f32_abs_bits(-0)", out32, f32_pzero);

        res = m3_CallV(f64_neg, f64_pzero); expect_m3_ok("call f64_neg_bits(+0)", res);
        m3_GetResultsV(f64_neg, &out64); expect_u64_eq("f64_neg_bits(+0)", out64, f64_nzero);

        res = m3_CallV(f64_abs, f64_nzero); expect_m3_ok("call f64_abs_bits(-0)", res);
        m3_GetResultsV(f64_abs, &out64); expect_u64_eq("f64_abs_bits(-0)", out64, f64_pzero);
    }

    // NaN-producing arithmetic (bit-pattern is not specified; only require NaN)
    {
        IM3Function f32_add = find_fn(runtime, "f32_add_bits");
        IM3Function f32_div = find_fn(runtime, "f32_div_bits");
        IM3Function f32_sqrt = find_fn(runtime, "f32_sqrt_bits");
        IM3Function f64_add = find_fn(runtime, "f64_add_bits");
        IM3Function f64_div = find_fn(runtime, "f64_div_bits");
        IM3Function f64_sqrt = find_fn(runtime, "f64_sqrt_bits");

        uint32_t out32 = 0;
        uint64_t out64 = 0;

        res = m3_CallV(f32_add, f32_pinf, f32_ninf); expect_m3_ok("call f32_add_bits(+inf,-inf)", res);
        m3_GetResultsV(f32_add, &out32); expect_u32_nan("f32_add_bits(+inf,-inf)", out32);

        res = m3_CallV(f32_div, f32_pzero, f32_pzero); expect_m3_ok("call f32_div_bits(0,0)", res);
        m3_GetResultsV(f32_div, &out32); expect_u32_nan("f32_div_bits(0,0)", out32);

        res = m3_CallV(f32_sqrt, f32_neg_one); expect_m3_ok("call f32_sqrt_bits(-1)", res);
        m3_GetResultsV(f32_sqrt, &out32); expect_u32_nan("f32_sqrt_bits(-1)", out32);

        res = m3_CallV(f64_add, f64_pinf, f64_ninf); expect_m3_ok("call f64_add_bits(+inf,-inf)", res);
        m3_GetResultsV(f64_add, &out64); expect_u64_nan("f64_add_bits(+inf,-inf)", out64);

        res = m3_CallV(f64_div, f64_pzero, f64_pzero); expect_m3_ok("call f64_div_bits(0,0)", res);
        m3_GetResultsV(f64_div, &out64); expect_u64_nan("f64_div_bits(0,0)", out64);

        // sqrt(-1.0) for f64
        const uint64_t f64_neg_one = 0xbff0000000000000ULL;
        res = m3_CallV(f64_sqrt, f64_neg_one); expect_m3_ok("call f64_sqrt_bits(-1)", res);
        m3_GetResultsV(f64_sqrt, &out64); expect_u64_nan("f64_sqrt_bits(-1)", out64);
    }

    // Float->int conversion: traps and saturating variants
    {
        IM3Function i32_trunc_s = find_fn(runtime, "i32_trunc_f32_s");
        IM3Function i32_trunc_u = find_fn(runtime, "i32_trunc_f32_u");
        IM3Function i32_trunc_sat_s = find_fn(runtime, "i32_trunc_sat_f32_s");
        IM3Function i32_trunc_sat_u = find_fn(runtime, "i32_trunc_sat_f32_u");

        IM3Function i64_trunc_s = find_fn(runtime, "i64_trunc_f64_s");
        IM3Function i64_trunc_u = find_fn(runtime, "i64_trunc_f64_u");
        IM3Function i64_trunc_sat_s = find_fn(runtime, "i64_trunc_sat_f64_s");
        IM3Function i64_trunc_sat_u = find_fn(runtime, "i64_trunc_sat_f64_u");

        uint32_t out32 = 0;
        uint64_t out64 = 0;

        // trapping i32.trunc_f32_s
        res = m3_CallV(i32_trunc_s, f32_qnan);
        expect_m3_err_eq("i32_trunc_f32_s(NaN)", res, m3Err_trapIntegerConversion);

        res = m3_CallV(i32_trunc_s, f32_pinf);
        expect_m3_err_eq("i32_trunc_f32_s(+inf)", res, m3Err_trapIntegerOverflow);

        res = m3_CallV(i32_trunc_s, f32_i32_2p31);
        expect_m3_err_eq("i32_trunc_f32_s(2^31)", res, m3Err_trapIntegerOverflow);

        res = m3_CallV(i32_trunc_s, f32_i32_min);
        expect_m3_ok("i32_trunc_f32_s(-2^31) call", res);
        if (res == m3Err_none) {
            m3_GetResultsV(i32_trunc_s, &out32);
            expect_u32_eq("i32_trunc_f32_s(-2^31)", out32, 0x80000000u);
        }

        // trapping i32.trunc_f32_u
        res = m3_CallV(i32_trunc_u, f32_qnan);
        expect_m3_err_eq("i32_trunc_f32_u(NaN)", res, m3Err_trapIntegerConversion);

        res = m3_CallV(i32_trunc_u, f32_pinf);
        expect_m3_err_eq("i32_trunc_f32_u(+inf)", res, m3Err_trapIntegerOverflow);

        res = m3_CallV(i32_trunc_u, 0xbf800000u); // -1.0f
        expect_m3_err_eq("i32_trunc_f32_u(-1.0)", res, m3Err_trapIntegerOverflow);

        res = m3_CallV(i32_trunc_u, f32_pzero);
        expect_m3_ok("i32_trunc_f32_u(0) call", res);
        if (res == m3Err_none) {
            m3_GetResultsV(i32_trunc_u, &out32);
            expect_u32_eq("i32_trunc_f32_u(0)", out32, 0);
        }

        // saturating i32.trunc_sat_f32_s
        res = m3_CallV(i32_trunc_sat_s, f32_qnan); expect_m3_ok("i32_trunc_sat_f32_s(NaN) call", res);
        m3_GetResultsV(i32_trunc_sat_s, &out32); expect_u32_eq("i32_trunc_sat_f32_s(NaN)", out32, 0);
        res = m3_CallV(i32_trunc_sat_s, f32_pinf); expect_m3_ok("i32_trunc_sat_f32_s(+inf) call", res);
        m3_GetResultsV(i32_trunc_sat_s, &out32); expect_u32_eq("i32_trunc_sat_f32_s(+inf)", out32, 0x7fffffffu);
        res = m3_CallV(i32_trunc_sat_s, f32_ninf); expect_m3_ok("i32_trunc_sat_f32_s(-inf) call", res);
        m3_GetResultsV(i32_trunc_sat_s, &out32); expect_u32_eq("i32_trunc_sat_f32_s(-inf)", out32, 0x80000000u);

        // saturating i32.trunc_sat_f32_u
        res = m3_CallV(i32_trunc_sat_u, f32_qnan); expect_m3_ok("i32_trunc_sat_f32_u(NaN) call", res);
        m3_GetResultsV(i32_trunc_sat_u, &out32); expect_u32_eq("i32_trunc_sat_f32_u(NaN)", out32, 0);
        res = m3_CallV(i32_trunc_sat_u, f32_pinf); expect_m3_ok("i32_trunc_sat_f32_u(+inf) call", res);
        m3_GetResultsV(i32_trunc_sat_u, &out32); expect_u32_eq("i32_trunc_sat_f32_u(+inf)", out32, 0xffffffffu);
        res = m3_CallV(i32_trunc_sat_u, f32_ninf); expect_m3_ok("i32_trunc_sat_f32_u(-inf) call", res);
        m3_GetResultsV(i32_trunc_sat_u, &out32); expect_u32_eq("i32_trunc_sat_f32_u(-inf)", out32, 0);
        res = m3_CallV(i32_trunc_sat_u, 0xbf800000u); expect_m3_ok("i32_trunc_sat_f32_u(-1.0) call", res);
        m3_GetResultsV(i32_trunc_sat_u, &out32); expect_u32_eq("i32_trunc_sat_f32_u(-1.0)", out32, 0);

        // trapping i64.trunc_f64_s / u
        res = m3_CallV(i64_trunc_s, f64_qnan);
        expect_m3_err_eq("i64_trunc_f64_s(NaN)", res, m3Err_trapIntegerConversion);
        res = m3_CallV(i64_trunc_s, f64_pinf);
        expect_m3_err_eq("i64_trunc_f64_s(+inf)", res, m3Err_trapIntegerOverflow);
        res = m3_CallV(i64_trunc_s, f64_i64_2p63);
        expect_m3_err_eq("i64_trunc_f64_s(2^63)", res, m3Err_trapIntegerOverflow);
        res = m3_CallV(i64_trunc_s, f64_i64_min);
        expect_m3_ok("i64_trunc_f64_s(-2^63) call", res);
        if (res == m3Err_none) {
            m3_GetResultsV(i64_trunc_s, &out64);
            expect_u64_eq("i64_trunc_f64_s(-2^63)", out64, 0x8000000000000000ULL);
        }

        res = m3_CallV(i64_trunc_u, f64_qnan);
        expect_m3_err_eq("i64_trunc_f64_u(NaN)", res, m3Err_trapIntegerConversion);
        res = m3_CallV(i64_trunc_u, f64_pinf);
        expect_m3_err_eq("i64_trunc_f64_u(+inf)", res, m3Err_trapIntegerOverflow);
        res = m3_CallV(i64_trunc_u, 0xbff0000000000000ULL); // -1.0
        expect_m3_err_eq("i64_trunc_f64_u(-1.0)", res, m3Err_trapIntegerOverflow);

        // saturating i64.trunc_sat_f64_s / u
        res = m3_CallV(i64_trunc_sat_s, f64_qnan); expect_m3_ok("i64_trunc_sat_f64_s(NaN) call", res);
        m3_GetResultsV(i64_trunc_sat_s, &out64); expect_u64_eq("i64_trunc_sat_f64_s(NaN)", out64, 0);
        res = m3_CallV(i64_trunc_sat_s, f64_pinf); expect_m3_ok("i64_trunc_sat_f64_s(+inf) call", res);
        m3_GetResultsV(i64_trunc_sat_s, &out64); expect_u64_eq("i64_trunc_sat_f64_s(+inf)", out64, 0x7fffffffffffffffULL);
        res = m3_CallV(i64_trunc_sat_s, f64_ninf); expect_m3_ok("i64_trunc_sat_f64_s(-inf) call", res);
        m3_GetResultsV(i64_trunc_sat_s, &out64); expect_u64_eq("i64_trunc_sat_f64_s(-inf)", out64, 0x8000000000000000ULL);

        res = m3_CallV(i64_trunc_sat_u, f64_qnan); expect_m3_ok("i64_trunc_sat_f64_u(NaN) call", res);
        m3_GetResultsV(i64_trunc_sat_u, &out64); expect_u64_eq("i64_trunc_sat_f64_u(NaN)", out64, 0);
        res = m3_CallV(i64_trunc_sat_u, f64_pinf); expect_m3_ok("i64_trunc_sat_f64_u(+inf) call", res);
        m3_GetResultsV(i64_trunc_sat_u, &out64); expect_u64_eq("i64_trunc_sat_f64_u(+inf)", out64, 0xffffffffffffffffULL);
        res = m3_CallV(i64_trunc_sat_u, f64_ninf); expect_m3_ok("i64_trunc_sat_f64_u(-inf) call", res);
        m3_GetResultsV(i64_trunc_sat_u, &out64); expect_u64_eq("i64_trunc_sat_f64_u(-inf)", out64, 0);
    }

    m3_FreeRuntime(runtime);
    m3_FreeEnvironment(env);

    if (g_failures == 0) {
        printf("PASS: fp edge tests (%" PRIu32 " bytes wasm)\n", wasm_fp_edge_wasm_len);
        return 0;
    }

    fprintf(stderr, "FAILED: %d fp edge tests\n", g_failures);
    return 1;
}
