


/**
 * 按概率分布填充行的工作内容
 * 行的开始对应工作列表的开头，行的末尾对应工作列表的末尾，且保持连续
 * @param {Array} row - 目标行数组
 * @param {string} role - 角色名
 * @param {number} numWorkCols - 工作列数
*/
function fillWorkRow(row, role, numWorkCols) {
  const manage_work = ["启动项目", "分析需求", "对使用软件提出意见",]

  const main_work = ["启动项目", "分析需求", "对使用软件提出意见",
    "流程工具搭建", "框架编写", "功能模块编写", "功能模块测试", "功能模块优化",
    "评估需求", "拆分模块", "编写文档",
    "编写测试报告", "编写用户手册", "编写开发文档", "编写维护文档", "编写培训资料",
    "培训运维人员", "培训开发人员", "培训测试人员", "培训管理人员", "修正bug",
    "ui开发", "ui测试", "ui优化", "ui设计", "ui评审", "ui修改", "白盒测试",
    "黑盒测试", "集成测试", "系统测试", "性能测试", "压力测试", "安全测试", "兼容性测试",
    "编写测试用例", "测试用例执行", "测试结果分析", "测试结果优化",
    "维护文档",
  ];
  const dev_work = ["拆分模块", "编写文档", "编写测试用例", "测试用例执行", "白盒测试",
    "黑盒测试", "集成测试", "系统测试", "性能测试", "压力测试", "安全测试", "兼容性测试",
    "维护文档", "编写用户手册", "编写开发文档", "编写维护文档", "编写培训资料",
    "培训运维人员", "培训开发人员", "培训测试人员", "培训管理人员", "修正bug",]

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

function Macro() {
  const sheet = Application.ActiveSheet;
  const targetRange = sheet.Range("C4", Application.Selection);

  const targetRows = targetRange.Rows.Count;
  const targetCols = targetRange.Columns.Count;
  const numWorkCols = targetCols - 1;

  // 前两个角色之外的随机角色池
  const otherRoles = ["开发", "测试", "运维"];
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