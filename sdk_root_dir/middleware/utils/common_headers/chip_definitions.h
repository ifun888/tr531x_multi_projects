/*
 * Copyright (c) Triductor. 2018-2020. All rights reserved.
 * Description:  Basic chip definitions
 * Author:
 * Create:  2018-10-15
 */
#ifndef LIB_COMMON_HEADERS_CHIP_DEFINITIONS_H
#define LIB_COMMON_HEADERS_CHIP_DEFINITIONS_H

/** @defgroup CHIP_Base CHIP Base Definitions
 * CHIP Base Definitions
 * @ingroup CHIP_Base
 * */
/** @defgroup CHIP_ARMCORE CHIP ARM Core Specifics
 * CHIP ARM Core Application Core Specifics
 * @ingroup CHIP_Base
 * @{
 * */
#define BT       0
#define PROTOCOL 1
#define APPS     2
#define GNSS     3
#define SECURITY 4
#define HIFI     PROTOCOL
#define WIFI     5
#define CONTROL_CORE 6
#define SENSOR   7

#define CM3      0
#define CM7      1
#define RISCV31  2
#define RISCV70  3
#define RISCV32  4

#define CHIP_LIBRA           (TARGET_CHIP_LIBRA)
#define CHIP_SOCMN1          (TARGET_CHIP_SOCMN1)
#define CHIP_TR5312            (TARGET_CHIP_TR5312)
#define CHIP_BRANDY          (TARGET_CHIP_BRANDY)
#define CHIP_SW39            (TARGET_CHIP_SW39)
#define CHIP_SW21            (TARGET_CHIP_SW21)
#define CHIP_SOCMN2          (TARGET_CHIP_SOCMN2)

#if (CHIP_LIBRA == 1)
#define MASTER_BY_ALL   APPS
#define CORE_NUMS 5 // Total cores in chip
#define CPU_NUM  3    // Total cpus in chip
#elif (CHIP_SOCMN1 == 1)
#define MASTER_BY_ALL   APPS
#define CORE_NUMS 4 // Total cores in chip
#define CPU_NUM  3    // Total cpus in chip
#elif (CHIP_TR5312 == 1)
#define MASTER_BY_ALL   APPS
#define CORE_NUMS 2 // Total cores in chip
#define CPU_NUM  2    // Total cpus in chip
#elif (CHIP_TR5310 == 1 || CHIP_TR5310P  == 1 || CHIP_TR5310PA  == 1 || \
       CHIP_TR5310PE == 1 || CHIP_TR5312L == 1 || CHIP_TR5316 == 1)
#define MASTER_BY_ALL   APPS
#define CORE_NUMS 1 // Total cores in chip
#define CPU_NUM  1    // Total cpus in chip
#elif (CHIP_TRSWGXX == 1)
#define MASTER_BY_ALL   APPS
#define CORE_NUMS 2 // Total cores in chip
#define CPU_NUM  2    // Total cpus in chip
#elif (CHIP_TR5336 == 1)
#define MASTER_BY_ALL   APPS
#define CORE_NUMS 1 // Total cores in chip
#define CPU_NUM  1    // Total cpus in chip
#elif (CHIP_BRANDY == 1)
#define MASTER_BY_ALL   APPS
#define CORE_NUMS 3 // Total cores in chip
#define CPU_NUM  3    // Total cpus in chip
#elif (CHIP_SW39 == 1)
#define MASTER_BY_ALL   APPS
#define CORE_NUMS 6 // Total cores in chip
#define CPU_NUM  3    // Total cpus in chip
#elif (CHIP_CAT1 == 1)
#define MASTER_BY_ALL   APPS
#define CORE_NUMS 2 // Total cores in chip
#define CPU_NUM  2    // Total cpus in chip
#elif (CHIP_SW21 == 1)
#define MASTER_BY_ALL   APPS
#define CORE_NUMS 5 // Total cores in chip
#define CPU_NUM  5    // Total cpus in chip
#elif (CHIP_SOCMN2 == 1)
#define MASTER_BY_ALL   APPS
#define CORE_NUMS 6 // Total cores in chip
#define CPU_NUM  3    // Total cpus in chip
#elif (CHIP_FHL2 == 1)
#define MASTER_BY_ALL   APPS
#define CORE_NUMS 1 // Total cores in chip
#define CPU_NUM  1    // Total cpus in chip
#else
#error Please define master control core
#endif

#define OTHER_CPU_NUM (CPU_NUM - 1) // Other cpus number, exclude current cpu.

#define MASTER_BY_LIBRA_ONLY        (CHIP_LIBRA && (CORE == APPS))
#define MASTER_BY_SOCMN1_ONLY       (CHIP_SOCMN1 && (CORE == APPS))
#define MASTER_BY_TR5312_ONLY         (CHIP_TR5312 && (CORE == APPS))
#define MASTER_BY_TR5310_ONLY         (CHIP_TR5310 && (CORE == APPS))
#define MASTER_BY_TR5310P_ONLY         (CHIP_TR5310P && (CORE == APPS))
#define MASTER_BY_TR5310PE_ONLY        (CHIP_TR5310PE && (CORE == APPS))
#define MASTER_BY_TR5310PA_ONLY        (CHIP_TR5310PA && (CORE == APPS))
#define MASTER_BY_TR5312L_ONLY         (CHIP_TR5312L && (CORE == APPS))
#define MASTER_BY_TR5316_ONLY         (CHIP_TR5316 && (CORE == APPS))
#define MASTER_BY_BRANDY_ONLY       (CHIP_BRANDY && (CORE == APPS))
#define MASTER_BY_TRSWGXX_ONLY         (CHIP_TRSWGXX && (CORE == APPS))
#define MASTER_BY_SW39_ONLY         (CHIP_SW39 && (CORE == APPS))
#define MASTER_BY_SW21_ONLY         (CHIP_SW21 && (CORE == APPS))
#define MASTER_BY_SOCMN2_ONLY       (CHIP_SOCMN2 && (CORE == APPS))


#define MASTER_ONLY                 (MASTER_BY_LIBRA_ONLY || MASTER_BY_SOCMN2_ONLY || \
                                     MASTER_BY_SOCMN1_ONLY || MASTER_BY_TR5312_ONLY || \
                                     MASTER_BY_BRANDY_ONLY || MASTER_BY_TR5310_ONLY || \
                                     MASTER_BY_TR5310P_ONLY || MASTER_BY_TR5310PA_ONLY || \
                                     MASTER_BY_TR5312L_ONLY || MASTER_BY_TR5316_ONLY || \
                                     MASTER_BY_TRSWGXX_ONLY || MASTER_BY_SW39_ONLY || \
                                     MASTER_BY_SW21_ONLY || MASTER_BY_TR5310PE_ONLY)

#define MCU_ONLY                    (MASTER_BY_LIBRA_ONLY || MASTER_BY_SOCMN1_ONLY || \
                                     MASTER_BY_TR5312_ONLY || MASTER_BY_BRANDY_ONLY || \
                                     MASTER_BY_TRSWGXX_ONLY || CHIP_TR5336 || CHIP_TR5310 || CHIP_TR5310P || CHIP_TR5310PE || \
                                     CHIP_TR5310PA || CHIP_TR5312L || CHIP_TR5316 || MASTER_BY_SW39_ONLY || \
                                     MASTER_BY_SW21_ONLY || MASTER_BY_SOCMN2_ONLY)


#define SLAVE_BY_LIBRA_BT          (CHIP_LIBRA && (CORE == BT))
#define SLAVE_BY_LIBRA_GNSS        (CHIP_LIBRA && (CORE == GNSS))
#define SLAVE_BY_LIBRA_SECURITY    (CHIP_LIBRA && (CORE == SECURITY))
#define SLAVE_BY_LIBRA_ONLY        (SLAVE_BY_LIBRA_BT || SLAVE_BY_LIBRA_GNSS || SLAVE_BY_LIBRA_SECURITY)
#define SLAVE_BY_SOCMN1_ONLY       (CHIP_SOCMN1 && (CORE == BT))
#define SLAVE_BY_SW39_BT           (CHIP_SW39 && (CORE == BT))
#define SLAVE_BY_TR5312_ONLY         (CHIP_TR5312 && (CORE == BT))
#define SLAVE_BY_BRANDY_BT         (CHIP_BRANDY && (CORE == BT))
#define SLAVE_BY_BRANDY_DSP        (CHIP_BRANDY && (CORE == DSP))
#define SLAVE_BY_BRANDY_ONLY       (SLAVE_BY_BRANDY_BT || SLAVE_BY_BRANDY_DSP)
#define SLAVE_BY_TRSWGXX_ONLY         (CHIP_TRSWGXX && (CORE == CONTROL_CORE))

#define CHIP_LIBRA_FPGA            (CHIP_LIBRA && (LIBRA_CHIP_FPGA))
#define CHIP_LIBRA_CS              (CHIP_LIBRA && (LIBRA_CHIP_CS))

#define CHIP_SOCMN1_FPGA             (CHIP_SOCMN1 && (SOCMN1_CHIP_FPGA))
#define CHIP_SOCMN1_V100             (CHIP_SOCMN1 && (SOCMN1_CHIP_V100))
#define CHIP_SOCMN1_V200             (CHIP_SOCMN1 && (SOCMN1_CHIP_V200))

#define CHIP_TR5312_FPGA              (CHIP_TR5312 && (TR5312_CHIP_FPGA))
#define CHIP_TR5312_V100              (CHIP_TR5312 && (TR5312_CHIP_V100))

#ifdef PRE_FPGA
    #define FPGA 1
    #define ASIC 0
#endif

#ifdef PRE_ASIC
    #define FPGA 0
    #define ASIC 1
#endif

#define CHIP_TR5310_FPGA          (CHIP_TR5310 && (FPGA))
#define CHIP_TR5310_V100          (CHIP_TR5310 && (ASIC))

#define CHIP_TR5310P_FPGA          (CHIP_TR5310P && (FPGA))
#define CHIP_TR5310P_V100          (CHIP_TR5310P && (ASIC))

#define CHIP_TR5310PE_FPGA          (CHIP_TR5310PE && (FPGA))
#define CHIP_TR5310PE_V100          (CHIP_TR5310PE && (ASIC))

#define CHIP_TR5310PA_FPGA          (CHIP_TR5310PA && (FPGA))
#define CHIP_TR5310PA_V100          (CHIP_TR5310PA && (ASIC))

#define CHIP_TR5312L_FPGA          (CHIP_TR5312L && (FPGA))
#define CHIP_TR5312L_V100          (CHIP_TR5312L && (ASIC))

#define CHIP_TR5316_FPGA          (CHIP_TR5316 && (FPGA))
#define CHIP_TR5316_V100          (CHIP_TR5316 && (ASIC))

#define CHIP_TR5336_FPGA          (CHIP_TR5336 && (FPGA))
#define CHIP_TR5336_V100          (CHIP_TR5336 && (ASIC))

#define CHIP_BRANDY_FPGA       (CHIP_BRANDY && (BRANDY_CHIP_FPGA))
#define CHIP_BRANDY_V100       (CHIP_BRANDY && (BRANDY_CHIP_V100))

#define CHIP_SW39_FPGA         (CHIP_SW39 && (SW39_CHIP_FPGA))
#define CHIP_SW39_V100         (CHIP_SW39 && (SW39_CHIP_V100))

#define CHIP_SOCMN2_FPGA       (CHIP_SOCMN2 && (SOCMN2_CHIP_FPGA))

#define CHIP_SW21_FPGA         (CHIP_SW21 && (SW21_CHIP_FPGA))
#define CHIP_SW21_V100         (CHIP_SW21 && (SW21_CHIP_V100))

#define CHIP_TRSWGXX_FPGA         (CHIP_TRSWGXX && (FPGA))
#define CHIP_TRSWGXX_V100          (CHIP_TRSWGXX && (ASIC))

#define CHIP_FPGA             (CHIP_LIBRA_FPGA || CHIP_SOCMN1_FPGA || CHIP_TR5312_FPGA || CHIP_SW39_FPGA || \
                               CHIP_BRANDY_FPGA || CHIP_TR5310_FPGA || CHIP_TR5310P_FPGA || CHIP_TR5310PA_FPGA || \
                               CHIP_TR5312L_FPGA || CHIP_TR5316_FPGA || CHIP_TR5336_FPGA || CHIP_TRSWGXX_FPGA || \
                               CHIP_SW21_FPGA || CHIP_TR5310PE_FPGA || CHIP_SOCMN2_FPGA)

#define CHIP_SOCMN1_ASIC       (CHIP_SOCMN1_V100 || CHIP_SOCMN1_V200)
#define CHIP_TR5312_ASIC        (CHIP_TR5312_V100)
#define CHIP_TR5310_ASIC        (CHIP_TR5310_V100)
#define CHIP_TR5310P_ASIC        (CHIP_TR5310P_V100)
#define CHIP_TR5310PE_ASIC        (CHIP_TR5310PE_V100)
#define CHIP_TR5310PA_ASIC        (CHIP_TR5310PA_V100)
#define CHIP_TR5312L_ASIC        (CHIP_TR5312L_V100)
#define CHIP_TR5316_ASIC        (CHIP_TR5316_V100)
#define CHIP_BRANDY_ASIC      (CHIP_BRANDY_V100)
#define CHIP_TR5336_ASIC        (CHIP_TR5336_V100)
#define CHIP_TRSWGXX_ASIC        (CHIP_TRSWGXX_V100)
#define CHIP_SW21_ASIC        (CHIP_SW21_V100)
#define CHIP_SW39_ASIC        (CHIP_SW39_V100)

#define CHIP_ASIC             (CHIP_LIBRA_CS || CHIP_SOCMN1_ASIC || \
                               CHIP_TR5312_ASIC || CHIP_BRANDY_ASIC || \
                               CHIP_TR5310_ASIC || CHIP_TR5310P_ASIC || \
                               CHIP_TR5310PA_ASIC || CHIP_TR5312L_ASIC || \
                               CHIP_TR5316_ASIC || CHIP_TR5336_ASIC || \
                               CHIP_TRSWGXX_ASIC || CHIP_SW21_ASIC || \
                               CHIP_TR5310PE_ASIC || CHIP_SW39_ASIC)

/** @} end of group CHIP_ARMCORE */
#endif
