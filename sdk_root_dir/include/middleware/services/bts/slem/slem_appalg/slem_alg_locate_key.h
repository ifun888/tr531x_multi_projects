/**
 * Copyright (c) Triductor. 2023-2024. All rights reserved.
 *
 * Description: app_alg api change\n
 * Author: Triductor \n
 * History: \n
 * 2024-04-10, Create file. \n
 */
#ifndef SLEM_ALG_LOCATE_KEY_H
#define SLEM_ALG_LOCATE_KEY_H

#include "posalg.h"
#include "slem_alg_common_para.h"
#include "slem_errcode.h"
/**
 * @if Eng
 * @brief  SLEM distance required for starting the inside and outside car identification algorithm.
 * @else
 * @brief  SLEM 车内外识别算法的启动距离。
 * @endif
 */
#define IN_OUT_START_DIS 3
/**
 * @if Eng
 * @brief  SLEM distance required for disabling the inside and outside car identification algorithm.
 * @else
 * @brief  SLEM 车内外识别算法的停用距离。
 * @endif
 */
#define IN_OUT_STOP_DIS 5
/**
 * @if Eng
 * @brief  SLEM number of slave anchors required for
 *         starting the inside and outside car identification algorithm.
 * @else
 * @brief  SLEM 启动车内外识别算法所需从锚点个数。
 * @endif
 */
#define ALG_CAR_IN_OUT_TRIGGER_NUM 2

/**
 * @if Eng
 * @brief  SLEM number of secondary anchors required for starting the positioning algorithms.
 * @else
 * @brief  SLEM 启动定位算法所需从锚点个数
 * @endif
 */
#define ALG_POS_TRIGGER_NUM 3

/**
 * @if Eng
 * @brief  SLEM number of anchors required for starting the positioning algorithms.
 * @else
 * @brief  SLEM 启动融合算法所需锚点个数。
 * @endif
 */
#define ALG_FUSION_TRIGGER_NUM 1
/**
 * @if Eng
 * @brief  SLEM number of training features required for inside
 *         and outside car identification algorithm.
 * @else
 * @brief  SLEM 车内外识别算法训练特征个数。
 * @endif
 */
#define ALG_ALL_FEATURE_NUM 15
/**
 * @if Eng
 * @brief  SLEM number of training features required for inside
 *         and outside car identification algorithm.
 * @else
 * @brief  SLEM 车内外识别算法训练特征个数。
 * @endif
 */
#define ALG_NV_NUM_OF_R 12
/**
 * @if Eng
 * @brief  SLEM maximum number of anchor points that can be configured for the nv.
 * @else
 * @brief  SLEM NV可以配置的最大锚点数量。
 * @endif
 */
#define ALG_NV_ANCHOR_NUM 7
/**
 * @if Eng
 * @brief  SLEM number of parameters configured for inside and outside identification.
 * @else
 * @brief  SLEM 车内外识别配置参数个数。
 * @endif
 */
#define CAR_IN_OUT_CFG_NUM 6
/**
 * @if Eng
 * @brief  SLEM dimension of positioning algorithm.
 * @else
 * @brief  SLEM 定位算法维度。
 * @endif
 */
#define POSALG_MEASURE_DEMISION 3
/**
 * @if Eng
 * @brief  SLEM the limit of anchor distance information failure.
 * @else
 * @brief  SLEM 锚点距离信息失效的界限。
 * @endif
 */
#define ALG_DIS_DATA_CLEAR_TRIGGER_NUM 3
/**
 * @if Eng
 * @brief  SLEM number of Invalid Location Attempts Triggering EKF Restart.
 * @else
 * @brief  SLEM 触发EKF重启的无效定位次数。
 * @endif
 */
#define ALG_EKF_INIT_THRESHOLD 6
/**
 * @if Eng
 * @brief  SLEM x-axis index.
 * @else
 * @brief  SLEM x轴脚标。
 * @endif
 */
#define AXIS_X_INDEX 0
/**
 * @if Eng
 * @brief  SLEM y-axis index.
 * @else
 * @brief  SLEM y轴脚标。
 * @endif
 */
#define AXIS_Y_INDEX 1
/**
 * @if Eng
 * @brief  SLEM z-axis index.
 * @else
 * @brief  SLEM z轴脚标。
 * @endif
 */
#define AXIS_Z_INDEX 2
/**
 * @if Eng
 * @brief  SLEM anchor A index.
 * @else
 * @brief  SLEM 锚点A脚标。
 * @endif
 */
#define ANCHOR_A_INDEX 0
/**
 * @if Eng
 * @brief  SLEM anchor B index.
 * @else
 * @brief  SLEM 锚点B脚标。
 * @endif
 */
#define ANCHOR_B_INDEX 1
/**
 * @if Eng
 * @brief  SLEM anchor C index.
 * @else
 * @brief  SLEM 锚点C脚标。
 * @endif
 */
#define ANCHOR_C_INDEX 2
/**
 * @if Eng
 * @brief  SLEM number 1000, used for the print of float number.
 * @else
 * @brief  SLEM 数字1000，用于log的float形式数据的打印。
 * @endif
 */
#define ALG_NUM_CARRY_1000 1000
/**
 * @if Eng
 * @brief  SLEM minimum precision, which is used to determine the value of the float type.
 * @else
 * @brief  SLEM 最小精度，用于float类型数值判断。
 * @endif
 */
#define POSALG_POS_ERR_ACCURACY 0.0001
/**
 * @if Eng
 * @brief  SLEM float 0.
 * @else
 * @brief  SLEM float类型数字0。
 * @endif
 */
#define FLOAT_ZERO 0.
/**
 * @if Eng
 * @brief  SLEM upper threshold for the number of invalid location results,
 *         which is one of the conditions for triggering forcible location and releasing space.
 * @else
 * @brief  SLEM 无效定位结果计数上限，触发强制定位以及释放空间的条件之一。
 * @endif
 */
#define COUNTER_NAN_POS_UPPER_LIMIT 15
/**
 * @if Eng
 * @brief  SLEM upper limit of the number of invalid in-vehicle and out-vehicle recognition results,
           which is one of the conditions for triggering space release.
 * @else
 * @brief  SLEM 无效车内外识别结果计数上限，触发释放空间的条件之一。
 * @endif
 */
#define COUNTER_CAR_IN_UPPER_LIMIT 7
/**
 * @if Eng
 * @brief  SLEM one of the conditions for triggering forcible positioning and releasing space.
 * @else
 * @brief  SLEM 触发强制定位以及释放空间的条件之一。
 * @endif
 */
#define ALG_NAN_POS_TRIGGER_MINDIS 3

/**
 * @if Eng
 * @brief  SLEM the latest data infomation.
 * @else
 * @brief  SLEM 最新到来的数据信息。
 * @endif
 */
typedef struct {
    uint8_t key_id;        /*!< @if Eng No. of current key.
                                             @else   当前钥匙的编号。 @endif */
    uint8_t anc_id;        /*!< @if Eng No. of current anchor.
                                             @else   当前锚点的编号。 @endif */
    bool refresh_ekf_flag; /*!< @if Eng whether restart positioning filter.
                                             @else   是否重启定位平滑。 @endif */
} slem_alg_data_info;
/**
 * @if Eng
 * @brief  SLEM information required by the inside and outside car identification and positioning algorithms.
 * @else
 * @brief  SLEM 车内外识别和定位算法所需信息。
 * @endif
 */
typedef struct {
    float config_pos_r[ALG_NV_NUM_OF_R]; /*!< @if Eng parameters of inside and outside car
                                                                 positioning algorithm.
                                                         @else   车内外定位算法参数。 @endif */
    bool enable_det_in_out;              /*!< @if Eng whether to enable the inside and outside
                                                                 car identification algorithm.
                                                         @else   是否开启车内外识别算法。 @endif */
    bool enable_pos_in_out;              /*!< @if Eng whether to enable the inside and outside
                                                                 car positioning algorithm.
                                                         @else   是否开启车内外定位算法。 @endif */
} slem_alg_config_pos_in_out;

/**
 * @if Eng
 * @brief  SLEM information required by the location algorithm
 * @else
 * @brief  SLEM 定位算法所需信息
 * @endif
 */
typedef struct {
    float x[ALG_NV_ANCHOR_NUM];              /*!< @if Eng x-axis information of each anchor point.
                                                     @else   各个锚点x轴信息。 @endif */
    float y[ALG_NV_ANCHOR_NUM];              /*!< @if Eng y-axis information of each anchor point.
                                                     @else   各个锚点y轴信息。 @endif */
    float z[ALG_NV_ANCHOR_NUM];              /*!< @if Eng z-axis information of each anchor point.
                                                     @else   各个锚点z轴信息。 @endif */
    float adjust_dis_pos[ALG_NV_ANCHOR_NUM]; /*!< @if Eng positioning correction value information.
                                                      of each anchor point //revise adjustment/correction.
                                                     @else   各个锚点定位修正值信息。 @endif */
} slem_alg_config_pos;

typedef struct {
    slem_tag_pos pos_result;
    int inout_result;  // 1:OUTSIDE 0:INSIDE
    float fusion_dis;
    int fusion_key;
    bool pos_flag;
    bool inout_flag;
    bool fusion_flag;
} slem_alg_locate_key_result;

typedef struct {
    slem_alg_config_car_in_out *parameters;     /*!< @if Eng parameters for configuring inside
                                                                     and outside identification algorithm.
                                                             @else   车内外识别算法配置参数。 @endif */
    slem_alg_config_pos_in_out *conf_pos_inout; /*!< @if Eng configuration parameters for inside and outside car
                                                                     identification and positioning algorithm. see @ref
                                                                     slem_alg_config_pos_in_out.
                                                             @else   车内外识别和车内外定位算法配置参数。 参考 @ref
                                                                     slem_alg_config_pos_in_out。 @endif */
    slem_alg_config_pos *con_ext;               /*!< @if Eng configuration parameters of positioning algorithm.
                                                                     see @ref slem_alg_config_pos.
                                                             @else   定位算法配置参数。
                                                                     参考 @ref slem_alg_config_pos。 @endif */
    uint8_t anchor_used_num_fusion;             /*!< @if Eng configuration parameters of door opening/closing
                                                                     algorithm.
                                                             @else   开关门算法配置参数。 @endif */
    uint8_t select_door_idx_method;                     /*!< @if Eng independent door opening/closing mode.
                                                             @else   独立开关门方式。 @endif */ // g_discbk_flag
} slem_alg_config_pack;

/**
 * @if Eng
 * @brief  Interface for merging and triggering the inside and outside positioning algorithm,
           inside and outside identification algorithm, and ranging fusion algorithm.
 * @par Description:
 * @attention  1.Not all three algorithms have results in each round.
 * @attention  2.If the positioning algorithm is disabled, the result accuracy of the
 *               ranging fusion algorithm is affected.
 * @param  [in]  dis                  basic information such as the current distance measurement
 *                                    and confidence of each anchor. see @ref slem_alg_dis_struct_t.
 * @param  [in]  conf_p               nv configuration algorithm information. see @ref slem_alg_config_pack.
 * @param  [in]  prifile_info         control parameters sent by the profile layer. see @ref slem_alg_profile_info.
 * @param  [in]  locate_key_result    Positioning results, inside and outside identification results, and distance
 *                                    values for opening and closing doors. see @ref slem_alg_locate_key_result.
 * @retval Execution Result Error Code.
 * @par Dependency:
 *            @li posalg.h
 *            @li pipeline.h
 *            @li slem_alg_common_para.h
 * @else
 * @brief  合并触发车内外定位算法，车内外识别算法以及测距融合算法接口。
 * @par 说明:
 * @attention  1.不一定每一轮三个算法都有结果。
 * @attention  2.若关闭定位算法则会影响测距融合算法结果精度。
 * @param  [in]  dis                各个锚点本轮测距，置信度等基础信息，参考 @ref slem_alg_dis_struct。
 * @param  [in]  conf_p             at配置算法信息，参考 @ref slem_alg_config_pack。
 * @param  [in]  prifile_info       profile层下发的控制参数，参考 @ref slem_alg_profile_info。
 * @param  [in]  locate_key_result  本轮定位结果，车内外识别结果以及用于开关门的距离值，参考 @ref slem_alg_locate_key_result。
 * @par Description:
 * @retval 执行结果错误码。
 * @par 依赖:
 *            @li posalg.h
 *            @li pipeline.h
 *            @li slem_alg_common_para.h
 * @endif
 */
errcode_slem slem_alg_locate_key(slem_alg_dis_info *dis, slem_alg_config_pack *conf_p, slem_alg_data_info *data_info,
                                 slem_alg_locate_key_result *locate_key_result);

#endif