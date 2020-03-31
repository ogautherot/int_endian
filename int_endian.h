/*
 * Copyright (c) Olivier Gautherot.
 *
 */

#ifndef INT_ENDIAN_H_
#define INT_ENDIAN_H_

#include <stdint.h>

using namespace std;

// ======================================================================

typedef int8_t int8_be;
typedef uint8_t uint8_be;
typedef int8_t int8_le;
typedef uint8_t uint8_le;

// x86/x64 generates more compact code with iterative swap (not unrolled)
// Cortex M0 (ARMv6-m) generates better code with unrolled loops
// Cortex M3 upwards has specific instructions
// ARM7 (ARMv4t) generates the same code
#ifdef __ARM_ARCH
#define __UNROLLED_SWAP__ 1
#endif

static inline uint16_t _generic_swap16(const uint16_t &v) {
#if defined(__ARM_ARCH) && (__ARM_ARCH >= 6)
    uint16_t ret;
    __asm__("  rev16 %0, %1" : "=r"(ret) : "r"(v));
    return ret;
#else
#ifdef __UNROLLED_SWAP__
    uint8_t *p = reinterpret_cast<uint8_t *> & v, tmp;
    tmp = p[0];
    p[0] = p[1];
    p[1] = tmp;
    return v;
#else
    return ((v & 0xff) << 8) | ((v & 0xff00) >> 8);
#endif
#endif
}

static inline uint32_t _generic_swap32(const uint32_t &v) {
#if defined(__ARM_ARCH) && (__ARM_ARCH >= 6)
    uint32_t ret = 0;
    __asm__("  rev %0, %1" : "=r"(ret) : "r"(v));
    return ret;
#else
#ifdef __UNROLLED_SWAP__
    uint8_t *p = reinterpret_cast<uint8_t *> & v, tmp;
    tmp = p[0];
    p[0] = p[3];
    p[3] = tmp;
    tmp = p[1];
    p[1] = p[2];
    p[2] = tmp;
    return v;
#else
    uint32_t ret = 0, tmp = v;
    for (int i = 0; i < 4; i++) {
        ret = (ret << 8) | (tmp & 0xff);
        tmp >>= 8;
    }
    return ret;
#endif
#endif
}

static inline uint64_t _generic_swap64(const uint64_t &v) {
#if defined(__ARM_ARCH) && (__ARM_ARCH >= 7)
    uint64_t ret = 0;
    uint32_t vlow = _generic_swap32(v & 0xffffffff);
    uint32_t vhigh = _generic_swap32(v >> 32);
    ret = ((uint64_t)vlow << 32) | vhigh;
    return ret;
#else
#ifdef __UNROLLED_SWAP__
    uint8_t *p = reinterpret_cast<uint8_t *> & v, tmp;
    tmp = p[0];
    p[0] = p[7];
    p[7] = tmp;
    tmp = p[1];
    p[1] = p[6];
    p[6] = tmp;
    tmp = p[2];
    p[2] = p[5];
    p[5] = tmp;
    tmp = p[3];
    p[3] = p[4];
    p[4] = tmp;
    return v;
#else
    uint64_t ret = 0, tmp = v;
    for (int i = 0; i < 8; i++) {
        ret = (ret << 8) | (tmp & 0xff);
        tmp >>= 8;
    }
    return ret;
#endif
#endif
}

// ======================================================================

template <typename _T, bool _is_big_endian> class _generic_endian {
 private:
    _T _raw;

    _T byte_order(const _T v) {
        _T ret = v;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        if (_is_big_endian) {
            ret = (sizeof(_T) == 2)
                      ? _generic_swap16(v)
                      : (sizeof(_T) == 4)
                            ? _generic_swap32(v)
                            : (sizeof(_T) == 8) ? _generic_swap64(v) : 0;
        }
#else
        if (!_is_big_endian) {
            ret = (sizeof(_T) == 2)
                      ? _generic_swap16(v)
                      : (sizeof(_T) == 4)
                            ? _generic_swap32(v)
                            : (sizeof(_T) == 8) ? _generic_swap64(v) : 0;
        }
#endif
        return ret;
    }

 public:
    /** Constructor
     */
    _generic_endian<_T, _is_big_endian>() : _raw(0) {}

    _generic_endian<_T, _is_big_endian>(const _T &val)
        : _raw(byte_order(val)) {}

    /** Copy the Big-Endian variable (copy constructor)
     * @param v Big-Endian variable
     * ***************************************************************/
    _generic_endian<_T, _is_big_endian>(
        const _generic_endian<_T, _is_big_endian> &v) {
        _raw = v._raw;
    }

    /** Assignment from host-order integer
     * @param v Integer value
     * @return Returns value in Big-Endian order
     * ***************************************************************/
    _generic_endian<_T, _is_big_endian> &operator=(const _T &v) {
        _raw = byte_order(v);
        return *this;
    }

    /** Cast from Big-Endian to host-order integer
     *
     */
    operator _T() { return byte_order(_raw); }

    _T raw() const { return _raw; }   // It's a bitmap, so unsigned
} __attribute__((packed));

typedef _generic_endian<int16_t, true> int16_be;
typedef _generic_endian<int16_t, false> int16_le;
typedef _generic_endian<uint16_t, true> uint16_be;
typedef _generic_endian<uint16_t, false> uint16_le;

typedef _generic_endian<int32_t, true> int32_be;
typedef _generic_endian<int32_t, false> int32_le;
typedef _generic_endian<uint32_t, true> uint32_be;
typedef _generic_endian<uint32_t, false> uint32_le;

typedef _generic_endian<int64_t, true> int64_be;
typedef _generic_endian<int64_t, false> int64_le;
typedef _generic_endian<uint64_t, true> uint64_be;
typedef _generic_endian<uint64_t, false> uint64_le;

// ======================================================================

template <bool _is_big_endian> class _generic_float {
 private:
    float _raw;

    float byte_order(float v) {
        union {
            uint32_t i;
            float f;
        } val, ret;
        val.f = v;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        if (_is_big_endian)
            ret.i = _generic_swap32(val.i);
#else
        if (!_is_big_endian)
            ret.i = _generic_swap32(val.i);
#endif
        val.i = ret.i;
        return val.f;
    }

 public:
    _generic_float<_is_big_endian>() {
        float f = 0.0;
        _raw = byte_order(f);
    }

    _generic_float<_is_big_endian>(const float &val) : _raw(byte_order(val)) {}

    _generic_float<_is_big_endian>(const _generic_float<_is_big_endian> &v) {
        _raw = v._raw;
    }

    _generic_float<_is_big_endian> &operator=(const float &v) {
        _raw = byte_order(v);
        return *this;
    }

    operator float() { return byte_order(_raw); }

    float raw() { return _raw; }   // It's a bitmap, so unsigned
} __attribute__((packed));

typedef _generic_float<true> float_be;
typedef _generic_float<false> float_le;

// ======================================================================

template <bool _is_big_endian> class _generic_double {
 private:
    double _raw;

    double byte_order(double v) {
        union {
            uint64_t i;
            double f;
        } val, ret;
        val.f = v;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        if (_is_big_endian)
            ret.i = _generic_swap64(val.i);
#else
        if (!_is_big_endian)
            ret.i = _generic_swap64(val.i);
#endif
        return val.f;
    }

 public:
    _generic_double<_is_big_endian>() {
        double f = 0.0;
        _raw = byte_order(f);
    }

    _generic_double<_is_big_endian>(const double &val)
        : _raw(byte_order(val)) {}

    _generic_double<_is_big_endian>(const _generic_double<_is_big_endian> &v) {
        _raw = v._raw;
    }

    _generic_double<_is_big_endian> &operator=(const double &v) {
        _raw = byte_order(v);
        return *this;
    }

    operator double() { return byte_order(_raw); }

    double raw() { return _raw; }   // It's a bitmap, so unsigned
} __attribute__((packed));

typedef _generic_double<true> double_be;
typedef _generic_double<false> double_le;

#endif   // INT_ENDIAN_H_
