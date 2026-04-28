#ifndef k210_it_h__
#define k210_it_h__
#include "car_platform.h"

typedef enum
{
    K210_CAMERA_LEFT = 1,
    K210_CAMERA_RIGHT = 2
} k210_camera_id_t;

typedef struct
{
    u8 camera_id;
    u8 valid_mask;
    u8 best_target;
    int stable_votes;
    u8 confidence;
    u16 x;
    u16 y;
    u8 seq;
    u8 fresh;
    u8 confidence_ok;
    u8 position_ok;
    u8 reliable;
    u8 has_valid_frame;
    long long int rx_time_ms;
} k210_observation_t;

extern int aaa,bbb;
extern u8 k210_left[];
extern u8 k210_right[];
extern int k210_cnt_l[];
extern int k210_cnt_r[];
extern u8 count_go;
extern int left_max,right_max;
extern u8 find_val1,find_val2,find_val3,find_val4;
extern int val1_cnt,val2_cnt,val3_cnt,val4_cnt;
extern u8 k210_left_conf;
extern u8 k210_right_conf;
extern u16 k210_left_x;
extern u16 k210_left_y;
extern u16 k210_right_x;
extern u16 k210_right_y;
extern u8 k210_left_seq;
extern u8 k210_right_seq;
extern volatile long long int k210_left_last_rx_time;
extern volatile long long int k210_right_last_rx_time;
extern volatile u8 k210_left_has_valid_frame;
extern volatile u8 k210_right_has_valid_frame;
extern volatile u16 k210_bad_frame_count;
extern u8 USART5_RX_BUF[];
extern u8 USART6_RX_BUF[];

/*
 * 功能: 解析左侧 K210 完整帧并刷新左相机 one-hot/置信度/坐标/投票统计。
 * 入参: 无，默认消费 USART5_RX_BUF 当前完整帧。
 * 出参: 无。
 * 异常: 帧非法时清空当前观测值并丢弃该帧，历史投票窗口不额外累加。
 * 边界: 每帧最多只给满足契约的激活类别累计一次，低置信度/脏坐标帧不会进入业务票池。
 */
void ZSP_DEAL1(void);

/*
 * 功能: 解析右侧 K210 完整帧并刷新右相机 one-hot/置信度/坐标/投票统计。
 * 入参: 无，默认消费 USART6_RX_BUF 当前完整帧。
 * 出参: 无。
 * 异常: 帧非法时直接丢弃。
 * 边界: 统计窗口由上层决定何时清零，本函数只做单帧增量更新。
 */
void ZSP_DEAL2(void);

/*
 * 功能: 返回指定相机当前累计票数最高的类别编号。
 * 入参: i=1 表示左相机，i=2 表示右相机。
 * 出参: 返回 1~8 的类别编号；无有效票时返回 0。
 * 异常: 非法相机编号时返回 0。
 * 边界: 只比较当前累计票数，不做阈值过滤。
 */
int find_max(u8 i);

/*
 * 功能: 对指定相机求出前两名类别及对应票数，供任务层做左右决策融合。
 * 入参: a=1 表示左相机，a=2 表示右相机。
 * 出参: 结果写入 find_val1~find_val4 与 val1_cnt~val4_cnt。
 * 异常: 非法相机编号时保持已有结果不变。
 * 边界: 仅输出当前窗口前两名，不负责清零历史计数。
 */
void find_MAX(u8 a);

/*
 * 功能: 清空左右相机的分类投票缓存。
 * 入参: 无。
 * 出参: 无。
 * 异常: 无。
 * 边界: 只清统计结果，不清最近一帧原始 one-hot/坐标信息。
 */
void K210_ResetVoteBuffers(void);

/*
 * 功能: 读取指定相机的正式观测对象，并附带 freshness/confidence/position 契约结果。
 * 入参: camera_id 为 K210_CAMERA_LEFT 或 K210_CAMERA_RIGHT；out 为输出对象。
 * 出参: 返回 1 表示读取成功，返回 0 表示入参非法。
 * 异常: 非法相机编号或空指针时不写输出。
 * 边界: 该函数只打包最新观测与当前票池最高目标，不修改历史状态。
 */
u8 K210_GetObservation(u8 camera_id, k210_observation_t *out);

/*
 * 功能: 判断指定相机最近一次观测是否满足 freshness/confidence/position 三重契约。
 * 入参: camera_id 为 K210_CAMERA_LEFT 或 K210_CAMERA_RIGHT。
 * 出参: 返回 1 表示可用于业务决策，返回 0 表示应视为不可靠观测。
 * 异常: 非法相机编号时返回 0。
 * 边界: 只检查最新观测契约，不代表累计投票窗口一定已经锁定。
 */
u8 K210_ObservationIsReliable(u8 camera_id);

#endif //k210_it_h__
