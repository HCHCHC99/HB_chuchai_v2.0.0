# 过流锁 bug 排查

## 现象

读故障寄存器 0x2740 = 0x0000（无过流故障），但正转和反转指令均无法转动电机。

## 方向锁机制

```
block_fwd: DEV_ID_OVERCUR_FWD, DEV_ID_RTURN_FWD
block_rev: DEV_ID_RTURN_REV
```

| 锁 | 队列 | 触发 | 清除 |
|----|------|------|------|
| DEV_ID_OVERCUR_FWD | block_fwd | 开窗(FWD)过流时 Motor_OnCurrentAlarm 加入 | 手动清故障 / 传感器自动恢复 |
| DEV_ID_RTURN_FWD | block_fwd | RTurn 发布 RTURN_LIMIT_FORWARD(u8IsActive=1) | 方向切换为 REV 时 RTurn 发布 u8IsActive=0 释放 |
| DEV_ID_RTURN_REV | block_rev | RTurn 发布 RTURN_LIMIT_REVERSE(u8IsActive=1) | 方向切换为 FWD 时 RTurn 发布 u8IsActive=0 释放 |

## 复现路径分析

### 场景 A：Dev_ID_OVERCUR_FWD 残留

```
1. 开窗(FWD)过流
   → block_fwd: DEV_ID_OVERCUR_FWD + DEV_ID_RTURN_FWD
   → fault=1

2. 用户发 0x2740=0x0002 清故障
   → Motor_OnCurrentAlarm: 移除 DEV_ID_OVERCUR_FWD
   → Motor_ClearOvercurrentBlock: 移除 DEV_ID_OVERCUR_FWD（双保险）
   → RealTime_ClearFault: 清除 fault bit
   → 但 DEV_ID_RTURN_FWD 未被清除！（RTurn 锁不走故障清除路径）

3. 结果: block_fwd 仍有 DEV_ID_RTURN_FWD, fault=0
   → FWD 转不动（被 RTurn 锁住）
   → REV 可以转（block_rev 为空）
```

此场景下仅 FWD 方向被锁，REV 可转。

### 场景 B：方向切换形成两向全锁

```
1. 开窗(FWD)过流 → fault=1, FWD 被锁

2. 不清故障，直接发 REV
   → Motor_OnCurrentAlarm: 方向=REV → 跳过（不锁 FWD）
   → 注意：DEV_ID_OVERCUR_FWD 仍在 block_fwd 中！
   → 方向切换 → RTurn 释放 FWD 锁 → DEV_ID_RTURN_FWD 移出
   → 但 DEV_ID_OVERCUR_FWD 仍在！
   → 电机反转，到达关窗极限 → REV 过流
   → 角度在阈值内 → TryCalibrate=true → 校准
   → RTURN_LIMIT_REVERSE → block_rev += DEV_ID_RTURN_REV

3. 结果:
   → block_fwd: DEV_ID_OVERCUR_FWD（从未清除）
   → block_rev: DEV_ID_RTURN_REV
   → fault=1（FWD 过流造成的，一直未清）

   两向全锁但 fault=1，非"无故障"场景。
```

### 场景 C：关窗过流在阈值内（校准成功）

这是**两向全锁 + fault=0** 的真正根因：

```
1. 电机在关窗极限位置附近（角度在 [-3°, -1°] 内）
   → 上电初始化：RTurn 锁状态为空

2. 用户发 REV → 电机反转（但其实已在极限，轻微动一下就堵转）
   → 过流检测
   → Motor_OnCurrentAlarm: 方向=REV → 跳过（不锁 FWD）
   → FaultHandler: 方向=REV → 跳过（不置故障位）
   → RTurn_HandleOvercurrent: 方向=REV
      → 角度在阈值内 → TryCalibrate=true → 校准
      → 发布 RTURN_LIMIT_REVERSE(u8IsActive=1)
      → 没有调用 RealTime_SetFault！
   → Motor_OnRTurnLimit: block_rev += DEV_ID_RTURN_REV

3. 结果:
   → block_fwd: 空
   → block_rev: DEV_ID_RTURN_REV
   → fault=0x0000

   仅 REV 被锁。FWD 可转。
```

此场景同样不是两向全锁。

### 场景 D：关窗过流在阈值外（报故障）+ 不清故障 + 方向切换

```
1. 电机角度在阈值外，发 REV → 关窗过流
   → Motor_OnCurrentAlarm: 方向=REV → 跳过
   → FaultHandler: 方向=REV → 跳过
   → RTurn_HandleOvercurrent: 方向=REV
      → 角度在阈值外 → TryCalibrate=false
      → RealTime_SetFault(FAULT_BIT_OVERCURRENT)
      → 发布 RTURN_LIMIT_REVERSE
   → Motor_OnRTurnLimit: block_rev += DEV_ID_RTURN_REV

   结果: block_rev 被锁, fault=1

2. 用户不清故障，发 FWD → 方向切换
   → RTurn 释放 REV 锁 → DEV_ID_RTURN_REV 移出
   → 电机正转 → 碰到正向极限 → FWD 过流
   → Motor_OnCurrentAlarm: 方向=FWD → block_fwd += DEV_ID_OVERCUR_FWD
   → FaultHandler: 方向=FWD → (fault bit 已经是 1，再次置位)
   → RTurn_HandleOvercurrent: 方向=FWD → RTURN_LIMIT_FORWARD
   → Motor_OnRTurnLimit: block_fwd += DEV_ID_RTURN_FWD

3. 结果:
   → block_fwd: DEV_ID_OVERCUR_FWD + DEV_ID_RTURN_FWD
   → block_rev: 空（已在步骤 2 释放）
   → fault=1

   两向全锁但 fault=1。
```

### 场景 E：核心根因——RTurn 锁随方向切换"翻转"形成两向全锁

```
1. 某项操作导致 FWD 方向有 RTurn 锁（DEV_ID_RTURN_FWD 在 block_fwd 中）
   fault=0（故障已清除或未触发故障位）

2. 用户发 REV → 方向切换
   → RTurn_UpdateAngle: locked=FWD, desired=REV → 释放 FWD 锁
   → Motor_OnRTurnLimit: 移除 DEV_ID_RTURN_FWD（u8IsActive=0）
   → 但同时 RTurn_UpdateAngle 轮询检测到仍有过流（电机还在机械极限位置）
   → 新方向 REV → 如果角度在校准阈值内 → 校准 → RTURN_LIMIT_REVERSE
   → Motor_OnRTurnLimit: block_rev += DEV_ID_RTURN_REV

3. 结果: 锁从 FWD "翻转"到了 REV。仅 REV 被锁。

4. 用户再发 FWD → 同样的翻转，锁回到 FWD。

   无论怎么翻，始终只有一个方向被锁。
```

## 结论

**经过遍历分析，未找到"两向全锁 + fault=0"的确切代码路径。**

最接近的场景是：
- `DEV_ID_RTURN_FWD` 残留导致 FWD 单向锁（场景 A）
- RTurn 锁随方向切换翻转到另一端

如果用户实际观察到的现象是"两向都转不动"且 fault=0，建议通过 RTT 日志确认以下信息：
1. `Motor_ArbitrationDecision` 的日志——看 block_fwd 和 block_rev 的队列内容
2. `Motor_OnRTurnLimit` 的日志——看锁的触发和释放时机
3. `Motor_OnCurrentAlarm` 的日志——看过流锁的触发和清除
