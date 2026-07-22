#!/usr/bin/env python3
"""Fix garbled comments in dev_motor.c/.h caused by double-encoding."""

DEV = r'D:\HB_chuchai_v.2.0.0\project\HC32F460_DDL_Rev3.3.0\projects\ev_hc32f460_lqfp100_v2\Dev'

FIXES = {
    'dev_motor.c': [
        # Header includes
        ('#include "dev_pwm.h"          // PWM豸\n', '#include "dev_pwm.h"          // PWM设备层\n'),
        ('#include "dev_voltage.h"      // ѹ澯¼\n', '#include "dev_voltage.h"      // 电压报警事件结构\n'),
        ('#include "dev_sensor.h"       // 澯¼\n', '#include "dev_sensor.h"       // 过流报警事件结构\n'),
        ('#include "dev_rturn.h"          // У\n', '#include "dev_rturn.h"          // 旋转限位: 含 RTurn_LimitEvent_t\n'),
        ('#include "Template_Pwm.h"       // PWMãTMRA_4, PB6/PB7\n', '#include "Template_Pwm.h"       // PWM配置: TMRA_4, PB6/PB7\n'),
        ('#include "hc32_ll_tmra.h"      // TMRAĴ\n', '#include "hc32_ll_tmra.h"      // TMRA寄存器操作\n'),
        # PWM section
        ('/* PWM ģʽ̬״̬', '/* PWM 模式静态状态变量 */'),
        ('/* 0м', '/* 极性切换: 先回0再切换极性 */'),
        ('/* Ŀռձ', '/* 待切换的目标占空比 */'),
        ('/* 1=FWD 2=REV */', '/* 方向: 1=FWD 2=REV */'),
        ('/ ͣʱ', '/* 缓启动/缓停时间(毫秒) */'),
        ('/* ռձ', '/* 占空比限制(2%-98%) */'),
        ('/* ռձ', '/* 占空比钳位函数 */'),
        ('/* ֹͣPB6~PB9 ȫ 98 Ч', '/* 停止: PB6~PB9 全 98% 低有效 */'),
        ('/* мȫΪЧ', '/* 运行极性: 全部设为低有效 */'),
        ('/* ֱ4ͨռձȣ̬ȫЧ', '/* 直接设置4通道占空比(全低有效) */'),
        ('/* -תCH1/2 ռձȣCH3/4 ռձ', '/* 缓启动-正转: CH1/2低占空比, CH3/4高占空比 */'),
        ('/* Ȼ0м', '/* 先缓回0, 极性切换后在Motor_Update中继续加速缓升 */'),
        ('/* ǰΪ0%ֱм', '/* 当前为0%: 直接切换极性并缓升 */'),
        ('/* ת޻', '/* 正转(无缓启动, 立即执行) */'),
        ('/* ת޻', '/* 反转(无缓启动, 立即执行) */'),
        # Debug
        ('// ӡ', '// 电机状态打印控制\n'),
        ('// ========== Keil Watch ȫֱ', '// ========== Keil Watch 调试全局变量 =========='),
        ('// ٲ', '// 仲裁器调试变量 - 在Watch中添加可实时查看仲裁状态\n'),
        ('// Ϊ', '// 辅助变量: 方便Watch查看\n'),
        # Device gene
        ('// ==========豸ID', '// ========== 设备ID枚举 =========='),
        ('//豸', '// 设备基因表\n'),
        # Arbitration callbacks
        ('// ========== ٲ', '// ========== 电机仲裁结果回调函数(用户可重写) =========='),
        ('// ʹ Template_Pwm.h APP_FUNC_FOUR_CH_LOW_PWM ģʽ', '// 使用 Template_Pwm.h 的 APP_FUNC_FOUR_CH_LOW_PWM 模式:\n//   TMRA_UNIT = CM_TMRA_4, CH1=PB6, CH2=PB7, CH3=PB8, CH4=PB9'),
        ('// תPB6=20%, PB7=20%, PB8=80%, PB9=80%ȫЧ', '// 正转: PB6=20%, PB7=20%, PB8=80%, PB9=80%(全低有效)'),
        ('// תPB6=80%, PB7=80%, PB8=20%, PB9=20%ȫЧ', '// 反转: PB6=80%, PB7=80%, PB8=20%, PB9=20%(全低有效)'),
        ('// ֹͣPB6=50%BBЧ, PB7=50%BBЧ, PB8=50%BBЧ, PB9=50%BBЧ', '// 停止: PB6=50%(高有效), PB7=50%(高有效), PB8=50%(高有效), PB9=50%(高有效)'),
        ('// ֹͣPB6=50%Ч, PB7=50%Ч, PB8=50%Ч, PB9=50%Ч', '// 停止: PB6=50%(高有效), PB7=50%(高有效), PB8=50%(高有效), PB9=50%(高有效)'),
        # Internal helpers
        ('// ==========ڲ', '// ========== 内部辅助函数 ==========\n'),
        # Motor_Init
        ('// IO豸Ĭ״̬ÿIO豸ĬBLOCKԼ', '// 初始化IO设备默认状态: 每个IO设备默认BLOCK自己的方向\n'),
        ('// IO_FWDĬBLOCKתIO_REVĬBLOCKת', '// IO_FWD默认BLOCK正转, IO_REV默认BLOCK反转\n'),
        # EventBus callbacks
        ('// ========== EventBusص', '// ========== EventBus回调函数 ==========\n'),
        ('// ========== ѹ澯ص', '// ========== 电压报警回调 ==========\n'),
        ('// ========== 澯ص', '// ========== 过流报警回调(已禁用) ==========\n'),
        # Standard interface
        ('// ========== 豸ӿ', '// ========== 标准设备接口(DeviceManager规范) ==========\n'),
        ('// ========== ضӿ', '// ========== 电机特定接口 ==========\n'),
        # Overcurrent alarm
        ('/* жϵǰ趨 תʱ', '/* 判断当前方向: 仅正转时阻塞正转\n         * 反转/停止时过流为预期工况(关窗到位), 不阻塞反转 */'),
        (' * תתתԤ', ''),  # remove duplicate garbled line
        ('// תʱ block_fwd DEV_ID_OVERCUR_FWD', '// 正转时过流: block_fwd += DEV_ID_OVERCUR_FWD\n'),
        ('// תֹͣʱ תתתԤ', '// 反转/停止时过流: 跳过(正常堵转, 不阻塞)\n'),
        ('// 澯 block_fwd DEV_ID_OVERCUR_FWD', '// 过流解除: 移除 block_fwd 中的 DEV_ID_OVERCUR_FWD\n'),
        # RTurn limit
        ('// ========== תλ', '// ========== 旋转限位回调 ==========\n'),
        ('/* λ趨', '/* 限位触发: 加入 BLOCK 指令(使用 RTurn 专用设备ID) */'),
        ('// ͬ allow_fwdȷʹ block Ȼת', '// 同时清空 allow_fwd\n'),
        # Hard limit
        ('// ========== Ӳλ', '// ========== 硬件限位回调 ==========\n'),
        # Power event
        ('// ========== Դ¼', '// ========== 电源事件回调 ==========\n'),
        ('// ========== ˫ģʽ', '// ========== 双极性模式 ==========\n'),
        ('// Դ豸ṩ ALLOW ת', '// 电源设备提供 ALLOW 到对应方向, 不产生 BLOCK\n'),
        ('// ========== ģʽ', '// ========== 单极性模式 ==========\n'),
        ('// ģʽµԴ豸', '// 单极性模式下电源设备不参与仲裁\n'),
        # Manual IO
        ('// ========== IO', '// ========== 手动IO事件回调 ==========\n'),
        ('// IO_FWD', '// IO_FWD按下: 移除block → 加入allow\n'),
        ('// IO_REV', '// IO_REV按下: 移除block → 加入allow\n'),
        ('// ֹͣIO豸', '// 停止所有IO: 移除allow → 加入block\n'),
        # CAN event
        ('// CAN¼', '// CAN事件回调(预留, 未实现)\n'),
        # Speed feedback
        ('// ========== תٷ', '// ========== 转速反馈回调 ==========\n'),
        # Clear interfaces
        ('// ========== ѹ澯ֶ', '// ========== 电压报警手动清除接口 ==========\n'),
        ('// ɵѹ澯ٲ', '// 清除电压报警在电机仲裁中设置的 block\n'),
        ('// ========== 澯ֶ', '// ========== 过流报警手动清除接口 ==========\n'),
        ('// ɵ', '// 手动清除过流block(保留作为双保险)\n'),
        # Motor specific
        ('//趨', '// 设置速度\n'),
        ('// ͨEventBus', '// 通过EventBus发布手动IO事件, 仲裁层处理\n'),
        ('// ֹͣ', '// 紧急停止: 清除所有 allow\n'),
        # Inline comments
        ('// PWM 豸', '// PWM 设备层'),
        ('// PWM  ', '// PWM '),
        # ===== device_manager.h 中已定义 =====
        ('// ע⣺CMD_BASE_MOTOR device_manager.h', '// 注意: CMD_BASE_MOTOR 在 device_manager.h 中定义'),
    ],

    'dev_motor.h': [
        # Config macros
        ('// ========== 豸', '// ========== 设备配置宏 =========='),
        ('// ģʽл', '// 模式切换: 0=单极性(单电源全桥), 1=双极性(双电源半桥)'),
        ('// ȼģʽ', '// 优先级模式: 1=IO高, 0=CAN高'),
        ('//豸', '// 设备能力配置(位标志)'),
        ('//豸', '// 设备能力位定义'),
        # Motor state struct (duplicate line)
        ('// ==========  ״̬ṹ', '// ========== 电机状态结构体(供 Device_Read 一次性读取) =========='),
        # Device ID enum
        ('// ==========豸ID', '// ========== 设备ID枚举 =========='),
        # Priority enum
        ('// ȼ', '// 优先级枚举'),
        # Command struct
        ('// ========== ', '// ========== '),
        # Debug info struct
        ('// ========== Ϣṹ', '// ========== 调试信息结构 =========='),
        # Motor device struct
        ('// ==========豸ṹ', '// ========== 电机设备结构体 =========='),
        # Event structs
        ('// ==========¼', '// ========== 事件数据结构(EventBus用) =========='),
        # callback declarations
        ('// ========== ٲ', '// ========== 仲裁回调(用户重写) =========='),
        ('// ٲ', '// 仲裁判定停止时回调'),
        ('// ٲ', '// 仲裁判定正转时回调'),
        ('// ٲ', '// 仲裁判定反转时回调'),
        # Device interface
        ('// ==========豸ӿ', '// ========== 电机设备接口(DeviceManager规范) =========='),
        ('// 豸', '// 标准设备操作'),
        ('// ضӿ', '// 电机特定接口'),
        ('// ֹͣӿ', '// 清除方向: 清除allow并加入block'),
        # Voltage/current alarm
        ('// ========== ѹ澯ֶ', '// ========== 电压报警手动清除 =========='),
        ('// ========== 澯ֶ', '// ========== 过流报警手动清除 =========='),
        # Keil Watch
        ('// ========== Keil Watch ȫ', '// ========== Keil Watch 调试变量 =========='),
    ],
}

import os

total = 0
for fname, fixes in FIXES.items():
    fpath = os.path.join(DEV, fname)
    with open(fpath, 'r', encoding='utf-8') as f:
        text = f.read()
    count = 0
    for old, new in fixes:
        if new and old in text:
            text = text.replace(old, new)
            count += 1
    with open(fpath, 'w', encoding='utf-8') as f:
        f.write(text)
    total += count
    print(f'{fname}: {count} fixes')

print(f'Total: {total} fixes')
