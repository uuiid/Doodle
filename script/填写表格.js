


/**
 * 按概率分布填充行的工作内容
 * 行的开始对应工作列表的开头，行的末尾对应工作列表的末尾，且保持连续
 * @param {Array} row - 目标行数组
 * @param {string} role - 角色名
 * @param {number} numWorkCols - 工作列数
*/
function fillWorkRow(row, role, numWorkCols) {
  const manage_work = ["启动项目", "分析需求", "ui评审", "对使用软件提出意见",]

  const main_work = ["启动项目", "评估需求", "功能模块编写", "功能模块优化",
    "培训人员", "修正bug", "ui设计",
    "ui开发", "ui优化", "ui修改", "修改bug", "调试优化", "修改bug", "调试优化", "修改bug", "调试优化", "代码审核", "重构代码",
    "工具编写"
  ];
  const dev_work = ["拆分模块", "编写测试用例", "测试用例执行",
    "编写黑盒测试", "编写集成测试", "编写模块代码", "编写模块代码", "白盒测试", "编写模块代码", "编写模块代码", "修正bug", "白盒测试", "修正bug", "修正bug",
    "培训运维人员", "培训测试人员", "培训管理人员", "修正bug",]

  const test_dev_work = [
    "分析同类软件", "准备测试资产", "黑盒测试", "集成测试", "系统测试", "性能测试", "压力测试", "安全测试", "兼容性测试",
    "线上测试", "编写测试用例", "测试用例执行", "测试结果分析", "测试结果优化", "编写用户手册", "编写开发文档", "编写维护文档",
    "维护文档",
  ]

  const operations_work = ["维护文档", "编写用户手册", "编写开发文档", "编写维护文档", "编写培训资料",
    "培训运维人员"
  ]
  const roles = {
    "管理": manage_work, "主程": main_work,
    "开发": dev_work, "测试": test_dev_work, "运维": operations_work

  };
  const workList = roles[role];
  if (!workList || workList.length === 0) return;
  const len = workList.length;

  let currentIdx = 0;

  for (let col = 0; col < numWorkCols; col++) {
    // 理想位置：随列从左到右线性增长
    const idealIdx = (col / numWorkCols) * (len - 1);

    // 落后于理想位置时，以一定概率前进 1 步，保持连续性
    if (currentIdx < len - 1) {
      const diff = idealIdx - currentIdx;
      const advanceProb = Math.min(1, diff * 0.6 + 0.15);
      if (Math.random() < advanceProb) {
        currentIdx++;
      }
    }

    row[col + 1] = workList[currentIdx];
  }
}
/**
 * 填充单个工作表
 * @param {Sheet} sheet - 目标工作表
 * @param {string} endCell - 结束单元格（如 "AU20"），起始格固定为 C4
 */
function fillSheet(sheet, endCell) {
  const targetRange = sheet.Range("C4", sheet.Range(endCell));
  const targetRows = targetRange.Rows.Count;
  const targetCols = targetRange.Columns.Count;
  const numWorkCols = targetCols - 1;

  // 前两个角色之外的随机角色池（带权重：运维概率较低）
  const otherRoles = ["开发", "测试", "开发", "测试", "开发", "测试", "开发", "测试", "运维"];
  // roles 中的 key 顺序，用于排序
  const roleOrder = ["管理", "主程", "开发", "测试", "运维"];

  // 创建目标大小的二维数组
  const result = new Array(targetRows);

  for (let i = 0; i < targetRows; i++) {
    result[i] = new Array(targetCols);

    // 第一列：角色分配
    let role;
    if (i === 0) {
      role = "管理";
    } else if (i === 1) {
      role = "主程";
    } else {
      role = otherRoles[Math.floor(Math.random() * otherRoles.length)];
    }
    result[i][0] = role;

    // 其余列：按概率分布填充工作内容
    fillWorkRow(result[i], role, numWorkCols);
  }

  // 保证"运维"至少出现一次（如果只有2行则无法插入其他角色，跳过）
  if (targetRows > 2) {
    const hasYunwei = result.slice(2).some(row => row[0] === "运维");
    if (!hasYunwei) {
      // 随机选一个非前两行的位置替换为"运维"
      const idx = 2 + Math.floor(Math.random() * (targetRows - 2));
      result[idx][0] = "运维";
      fillWorkRow(result[idx], "运维", numWorkCols);
    }
  }

  // 对第 3 行起按角色顺序排序
  const tail = result.slice(2);
  tail.sort((a, b) => roleOrder.indexOf(a[0]) - roleOrder.indexOf(b[0]));
  for (let i = 2; i < targetRows; i++) {
    result[i] = tail[i - 2];
  }

  // 一次性写入目标区域
  targetRange.Value2 = result;

  // 设置格式（居中对齐、自动换行）
  targetRange.HorizontalAlignment = xlHAlignCenter;
  targetRange.VerticalAlignment = xlVAlignCenter;
  targetRange.WrapText = true;
}

function Macro() {
  const sheet_end_map = {
    rd15: "BZ20",
    // rd16: "AU18",
    // rd17: "BM20",
    // rd18: "AZ15",
    // rd19: "BD21",
    // rd20: "AV17",
    // rd21: "AV21",
    rd22: "BI18",
    // rd23: "BA22",
    // rd24: "BD34",
    // rd25: "AQ25",
    // rd26: "AZ25",
    // rd27: "AQ20",
    // rd28: "AR16",
    // rd29: "BE27",
    // rd30: "BE25",
    // rd31: "BE34",
    // rd32: "AM24",
    // rd33: "AN23",
    // "rd34-01": "AV23",
    // "rd34-02": "AV22",
    // "rd34-03": "AQ25",
    // "rd34-04": "AQ26",
    // "rd34-05": "AQ29",
    // rd35: "AN21",
    // rd36: "AR34",
    // rd37: "AR26",
  };
  for (const [sheetName, endCell] of Object.entries(sheet_end_map)) {
    let sheet;
    try {
      sheet = Application.Workbooks.Item(1).Sheets(sheetName);
    } catch (e) {
      // 如果找不到对应名称的工作表，跳过
      continue;
    }

    fillSheet(sheet, endCell);
  }
}
