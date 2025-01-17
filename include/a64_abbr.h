#ifndef _A64_ABBR_H_
#define _A64_ABBR_H_

#define BEQ(imm) a64_b(EQ, (imm))
#define BNE(imm) a64_b(NE, (imm))
#define BCS(imm) a64_b(CS, (imm))
#define BCC(imm) a64_b(CC, (imm))
#define BMI(imm) a64_b(MI, (imm))
#define BPL(imm) a64_b(PL, (imm))
#define BVS(imm) a64_b(VS, (imm))
#define BVC(imm) a64_b(VC, (imm))
#define BHI(imm) a64_b(HI, (imm))
#define BLS(imm) a64_b(LS, (imm))
#define BGE(imm) a64_b(GE, (imm))
#define BLT(imm) a64_b(LT, (imm))
#define BGT(imm) a64_b(GT, (imm))
#define BLE(imm) a64_b(LE, (imm))
#define BAL(imm) a64_b(AL, (imm))
#define BNV(imm) a64_b(NV, (imm))
#define B(imm) a64_b(B, (imm))

#define BR a64_br
#define CBZW a64_cbzw
#define CBZ a64_cbz
#define CBNZW a64_cbnzw
#define CBNZ a64_cbnz

#define BL a64_bl

#define BLI a64_bli
#define BLR a64_blR

#define CMPIW a64_cmpiw
#define CMPI a64_cmpi

#define MOV a64_mov
#define MOVW a64_movw
#define MOVI a64_movi
#define MOVIW a64_moviw

#define ADDI a64_addi
#define SUBI a64_subi
#define ADR a64_adr

#define ADC a64_adc
#define ADCW a64_adcw
#define ADCS a64_adcs
#define ADCSW a64_adcsw
#define SBC a64_sbc
#define SBCW a64_sbcw
#define SBCS a64_sbcs
#define SBCSW a64_sbcsw
#define ADD a64_add
#define ADDW a64_addw
#define ADDS a64_adds
#define ADDSW a64_addsw
#define SUB a64_sub
#define SUBW a64_subw
#define SUBS a64_subs
#define SUBSW a64_subsw

#define CMP a64_cmp
#define CMPW a64_cmpw

#define _CAT(x, y) x ## y
#define _XCAT(x, y) _CAT(x, y)

#define _OPT_OFF(...) _OPT_OFF_HELPER((__VA_ARGS__) __VA_OPT__(,) , 0)
#define _OPT_OFF_HELPER(arg, N, ...) _XCAT(_OPT_OFF_ARG, N)(arg)
#define _OPT_OFF_ARG0(arg) 0
#define _OPT_OFF_ARG(arg) arg


#define LDR(mode, dst, addr, ...)	a64_load_store(LOAD, mode, SZX, false, dst, addr, _OPT_OFF(__VA_ARGS__))
#define LDRW(mode, dst, addr, ...)	a64_load_store(LOAD, mode, SZW, false, dst, addr, _OPT_OFF(__VA_ARGS__))
#define LDRH(mode, dst, addr, ...)	a64_load_store(LOAD, mode, SZH, false, dst, addr, _OPT_OFF(__VA_ARGS__))
#define LDRB(mode, dst, addr, ...)	a64_load_store(LOAD, mode, SZB, false, dst, addr, _OPT_OFF(__VA_ARGS__))
#define LDRS(mode, dst, addr, ...)	a64_load_store(LOAD, mode, SZX, true, dst, addr, _OPT_OFF(__VA_ARGS__))
#define LDRSW(mode, dst, addr, ...)	a64_load_store(LOAD, mode, SZW, true, dst, addr, _OPT_OFF(__VA_ARGS__))
#define LDRSH(mode, dst, addr, ...)	a64_load_store(LOAD, mode, SZH, true, dst, addr, _OPT_OFF(__VA_ARGS__))
#define LDRSB(mode, dst, addr, ...)	a64_load_store(LOAD, mode, SZB, true, dst, addr, _OPT_OFF(__VA_ARGS__))

#define STR(mode, dst, addr, ...)	a64_load_store(STORE, mode, SZX, false, dst, addr, _OPT_OFF(__VA_ARGS__))
#define STRW(mode, dst, addr, ...)	a64_load_store(STORE, mode, SZW, false, dst, addr, _OPT_OFF(__VA_ARGS__))
#define STRH(mode, dst, addr, ...)	a64_load_store(STORE, mode, SZH, false, dst, addr, _OPT_OFF(__VA_ARGS__))
#define STRB(mode, dst, addr, ...)	a64_load_store(STORE, mode, SZB, false, dst, addr, _OPT_OFF(__VA_ARGS__))
#define STRS(mode, dst, addr, ...)	a64_load_store(STORE, mode, SZX, true, dst, addr, _OPT_OFF(__VA_ARGS__))
#define STRSW(mode, dst, addr, ...)	a64_load_store(STORE, mode, SZW, true, dst, addr, _OPT_OFF(__VA_ARGS__))
#define STRSH(mode, dst, addr, ...)	a64_load_store(STORE, mode, SZH, true, dst, addr, _OPT_OFF(__VA_ARGS__))
#define STRSB(mode, dst, addr, ...)	a64_load_store(STORE, mode, SZB, true, dst, addr, _OPT_OFF(__VA_ARGS__))

#define RET a64_ret
#define NOP a64_nop
#define BRK a64_brk

#define SZ_Q_V8B SIMD_B, SIMD_HALF
#define SZ_Q_V16B SIMD_B, SIMD_FULL
#define SZ_Q_V4H SIMD_H, SIMD_HALF
#define SZ_Q_V8H SIMD_H, SIMD_FULL
#define SZ_Q_V2S SIMD_S, SIMD_HALF
#define SZ_Q_V4S SIMD_S, SIMD_FULL
#define SZ_Q_V1D SIMD_D, SIMD_HALF
#define SZ_Q_V2D SIMD_D, SIMD_FULL

#define LD1(ot, mode, ...) a64_simd_ld1(mode, _XCAT(SZ_Q_, ot), __VA_ARGS__)

#define LD1B8(mode, ...)	a64_simd_ld1(mode, SIMD_B, SIMD_HALF, __VA_ARGS__)
#define LD1B16(mode, ...)	a64_simd_ld1(mode, SIMD_B, SIMD_FULL, __VA_ARGS__)
#define LD1H4(mode,  ...)	a64_simd_ld1(mode, SIMD_H, SIMD_HALF, __VA_ARGS__)
#define LD1H8(mode,  ...)	a64_simd_ld1(mode, SIMD_H, SIMD_FULL, __VA_ARGS__)
#define LD1S2(mode, ...)	a64_simd_ld1(mode, SIMD_S, SIMD_HALF, __VA_ARGS__)
#define LD1S4(mode,  ...)	a64_simd_ld1(mode, SIMD_S, SIMD_FULL, __VA_ARGS__)
#define LD1D1(mode,  ...)	a64_simd_ld1(mode, SIMD_D, SIMD_HALF, __VA_ARGS__)
#define LD1D2(mode,  ...)	a64_simd_ld1(mode, SIMD_D, SIMD_FULL, __VA_ARGS__)

#define SIMD_ANDF(dst, s1, s2) a64_simd_and(SIMD_FULL, dst, s1, s2)
#define SIMD_ANDH(dst, s1, s2) a64_simd_and(SIMD_HALF, dst, s1, s2)

#define SIMD_NOTF(dst, s1) a64_simd_not(SIMD_FULL, dst, s1)
#define SIMD_NOTH(dst, s1) a64_simd_not(SIMD_HALF, dst, s1)

#define SIMD_MVNF(dst) ({a64_t _rdst = (dst);a64_simd_not(SIMD_FULL, _rdst, _rdst);})
#define SIMD_MVNH(dst) ({a64_t _rdst = (dst);a64_simd_not(SIMD_HALF, _rdst, _rdst);})



#define SIMD_CMEQ(ot, dst, s1, s2)	a64_simd_cmeq(_XCAT(SZ_Q_, ot), dst, s1, s2)
#define SIMD_CMHI(ot, dst, s1, s2)	a64_simd_cmhi(_XCAT(SZ_Q_, ot), dst, s1, s2)
#define SIMD_CMHS(ot, dst, s1, s2)	a64_simd_cmhs(_XCAT(SZ_Q_, ot), dst, s1, s2)
#define SIMD_UMAXP(ot, dst, s1, s2)	a64_simd_umaxp(_XCAT(SZ_Q_, ot), dst, s1, s2)
#define SIMD_UMINP(ot, dst, s1, s2)	a64_simd_uminp(_XCAT(SZ_Q_, ot), dst, s1, s2)
#define SIMD_UMAXV(ot, dst, s1)		a64_simd_umaxv(_XCAT(SZ_Q_, ot), dst, s1)
#define SIMD_UMINV(ot, dst, s1)		a64_simd_uminv(_XCAT(SZ_Q_, ot), dst, s1)


#define SIMD_UMOVB(dst, s, idx)	a64_simd_umov(SIMD_B, idx, dst, src)
#define SIMD_UMOVH(dst, s, idx)	a64_simd_umov(SIMD_H, idx, dst, src)
#define SIMD_UMOVS(dst, s, idx)	a64_simd_umov(SIMD_S, idx, dst, src)
#define SIMD_UMOVD(dst, s, idx)	a64_simd_umov(SIMD_D, idx, dst, src)

#define SIMD_UMOVI8B(dst, imm)	a64_simd_movib(SIMD_HALF, dst, imm)
#define SIMD_UMOVI16B(dst, imm)	a64_simd_movib(SIMD_FULL, dst, imm)


#endif /* _A64_ABBR_H_ */