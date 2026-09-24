# 提交规范

本仓库使用 fork + branch + pull request。候选人不得直接 push 组织者主仓库。

推荐流程：

```bash
git clone <your-fork-url>
cd robomaster-assessment
git checkout -b candidate/f3-target-manager
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
git add src tests docs
git commit -m "F3: add target lock hysteresis"
git push -u origin candidate/f3-target-manager
```

然后向组织者主仓库创建 PR。一个 PR 只做一道题；如果需要讨论设计，可以先创建 Draft PR。组织者会运行隐藏测试、检查提交范围和路径依赖，再决定是否合并。
